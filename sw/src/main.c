
#include "MKL26Z4.h"
#include "systick.h"
#include <string.h> // memset
#include "usb_main.h"
#include <stdlib.h> // strtoul
#include <stdbool.h>
#include "rgb_mgr.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

// makes heap go
caddr_t
_sbrk (int incr)
{
  extern char   end __asm ("end");
  extern char   heap_limit __asm ("__HeapLimit");
  static char * heap_end;
  char *        prev_heap_end;

  if (heap_end == NULL)
    heap_end = & end;

  prev_heap_end = heap_end;

  if (heap_end + incr > &heap_limit)
    {
#ifdef NIO_ENOMEM   //TODO: Update NIO error code for MQX
        errno = NIO_ENOMEM;
#else
        errno = ENOMEM;
#endif
      return (caddr_t) -1;
    }

  heap_end += incr;

  return (caddr_t) prev_heap_end;
}

// dead area, used as reset pulse in 1-wire ws2812 interface
#define SPI_DEAD_PULSE_LENGTH 260
static uint8_t spiData[SPI_DEAD_PULSE_LENGTH + 3 * 3 * RGB_MGR_PANEL_COUNT *
                       RGB_MGR_LEDS_PER_PANEL];

static void main_init_io(void)
{
  // init ports
  // disable COP
  SIM_COPC = 0;

  // enable clocks for PORTB
  SIM_SCGC5 |= SIM_SCGC5_PORTB_MASK;

  // set B0 to GPIO
  PORTB_PCR0 = PORT_PCR_MUX(1);

  // set output B0 low (LED on initially)
  GPIOB_PCOR = (1 << 0);

  // set B0 DDR to output
  GPIOB_PDDR |= (1 << 0);
}

// static void delay_ms(uint32_t ms)
// {
//    uint32_t start = systick_getms();
//    while(systick_getms() - start < ms);
// }

static void main_init_uart(void)
{
  uint32_t baud;

  // init uart0
  // enable clocks for port a and uart0
  SIM_SCGC5 |= SIM_SCGC5_PORTA_MASK;
  SIM_SCGC4 |= SIM_SCGC4_UART0_MASK;
  SIM_SOPT2 |= SIM_SOPT2_UART0SRC(1); // select MCGFLLCLK clock

  // configure io pins for uart0- alt 2
  PORTA_PCR1 = PORT_PCR_MUX(2);
  PORTA_PCR2 = PORT_PCR_MUX(2);

  // set output mode for TX, pa12
  GPIOA_PDDR |= (1 << 2);

  // set oversampling ratio to 25
  UART0_C4 = UART0_C4_OSR(25 - 1);

  // set baud rate to 9600
  baud      = (48000000 / 9600) / 25;            // <- baud = CLOCK /
                                                 // ((OSR+1)*BR)
  UART0_BDH = UART0_BDH_SBR((baud >> 8) & 0x1F); // upper 5 bits
  UART0_BDL = UART0_BDL_SBR(baud & 0xFF);        // lower 8 bits

  // turn on tx + rx
  UART0_C2 = UART0_C2_RE_MASK | UART0_C2_TE_MASK;

  //    // send a char?
  //    UART0_D = 'N';
}

static void main_init_spi(void)
{
  // init spi0
  // enable clocks for PORTC
  SIM_SCGC5 |= SIM_SCGC5_PORTC_MASK;

  // configure io pins for spi- alt 2
  // PTC4-5-6
  PORTC_PCR4 = PORT_PCR_MUX(2);
  PORTC_PCR5 = PORT_PCR_MUX(2);
  PORTC_PCR6 = PORT_PCR_MUX(2);

  // set pc6 to output-
  GPIOC_PDDR |= (1 << 6);

  // enable SPI0 module
  SIM_SCGC4 |= SIM_SCGC4_SPI0_MASK;

  // configure as master, cs output driven automatically, CPOL=1
  SPI0_C1 |= (SPI_C1_MSTR_MASK | SPI_C1_SSOE_MASK | SPI_C1_CPOL_MASK);
  SPI0_C2 |= SPI_C2_MODFEN_MASK;

  // select clock divider- SPPR = 0b010 (/3), SPR = 0b0010 (/8).
  // target baud rate is 2MHz
  SPI0_BR = SPI_BR_SPPR(0x4) | SPI_BR_SPR(0x0);

  // turn on spi
  SPI0_C1 |= SPI_C1_SPE_MASK;

  // now set up dma
  SIM_SCGC6 |= SIM_SCGC6_DMAMUX_MASK; // enable clocks
  SIM_SCGC7 |= SIM_SCGC7_DMA_MASK;

  // dma mux
  // disable dmamux0 & clear channel select
  DMAMUX0_BASE_PTR->CHCFG[0] &=
    ~(DMAMUX_CHCFG_ENBL_MASK | DMAMUX_CHCFG_SOURCE_MASK);

  // select channel 17 for spi tx & enable it
  DMAMUX0_BASE_PTR->CHCFG[0] |= DMAMUX_CHCFG_SOURCE(17) | DMAMUX_CHCFG_ENBL_MASK;

  // dma
  // set destination address register to the SPI tx register
  DMA_BASE_PTR->DMA[0].DAR = (uint32_t)&SPI0_DL;

  // configure DMA_0 for spi tx
  DMA_BASE_PTR->DMA[0].DCR = DMA_DCR_ERQ_MASK | // enable periph. request
                             DMA_DCR_CS_MASK |
                             DMA_DCR_SINC_MASK |
                             DMA_DCR_SSIZE(0x01) |
                             DMA_DCR_DSIZE(0x01) |
                             DMA_DCR_D_REQ_MASK;
}

static void main_spi_send(uint8_t *pData, uint32_t len)
{
  // wait until dma busy flag is cleared
  while (DMA_BASE_PTR->DMA[0].DSR_BCR & DMA_DSR_BCR_BSY_MASK) ;

  //    // pull spi mosi low (p27) for at least 50us
  //    PORTC_PCR6 = PORT_PCR_MUX(1);
  //    GPIOC_PCOR = (1 << 6);
  //    delay_ms(2);
  // reset dma
  DMA_BASE_PTR->DMA[0].DSR_BCR = DMA_DSR_BCR_DONE_MASK;

  // write to the dma
  DMA_BASE_PTR->DMA[0].SAR     = (uint32_t)pData;
  DMA_BASE_PTR->DMA[0].DSR_BCR = len & 0x00ffffff;

  // enable dma on the spi
  DMA_BASE_PTR->DMA[0].DCR |= DMA_DCR_ERQ_MASK;

  // PORTC_PCR6 = PORT_PCR_MUX(2);
  SPI0_C2 |= SPI_C2_TXDMAE_MASK;

  //    // wait until dma busy flag is cleared
  //    while(DMA_BASE_PTR->DMA[0].DSR_BCR & DMA_DSR_BCR_BSY_MASK);
}
#define DEFAULT_TIME_STR ("1200")
static char* main_get_time(void)
{
  static char ascii_time[sizeof(DEFAULT_TIME_STR)] = DEFAULT_TIME_STR;

  sprintf(ascii_time, "%2lu%02lu", (systick_getms() / 10 / 60 / 60) % 12,
          (systick_getms() / 10 / 60) % 60);

  return ascii_time;
}

static void main_led(void)
{
  static uint32_t blinkTime = 0;

  // blink every 250ms
  if (systick_getms() - blinkTime > 250) {
    blinkTime = systick_getms();

    // toggle
    GPIOB_PTOR = (1 << 0);
  }
}

static int main_poll_uart(uint8_t *pChr)
{
  if (UART0_S1 & UART0_S1_RDRF_MASK) {
    *pChr = UART0_D;
    return 1;
  }

  return -1;
}

int main(void)
{
  #define SIZE_OF_CHARBUF 16
  static uint32_t rgbTime = 0;
  uint8_t   charBuf[SIZE_OF_CHARBUF];
  uint8_t   settingCount;
  bool      settingActive = false;
  bool      inputActive   = false;
  uint8_t   rxData;
  int       gotCharacter = -1;
  rgbData_t tempColor;
  bool scrollactive = false, testactive = false, incrementactive = false;

  // initialize the necessary
  main_init_io();
  main_init_spi();
  main_init_uart();

  // rgb init
  memset(spiData, 0, sizeof(spiData));
  rgb_mgr_init(&spiData[SPI_DEAD_PULSE_LENGTH],
               sizeof(spiData) - SPI_DEAD_PULSE_LENGTH);
  rgb_mgr_set_new_str((uint8_t*)main_get_time(), sizeof(DEFAULT_TIME_STR) - 1);
  rgb_mgr_set_scroll(false);

  // usb init
  usb_main_init();

  while (1) {
    // led task
    main_led();

    // rgb task
    rgb_mgr_main_function();

    // refresh every 50 ms
    if (systick_getms() - rgbTime > 50) {
      rgbTime = systick_getms();
      main_spi_send(spiData, sizeof(spiData));
      rgb_mgr_set_new_str((uint8_t*)main_get_time(), sizeof(DEFAULT_TIME_STR) - 1);
    }

    // usb task
    gotCharacter = usb_main_mainfunction(&rxData);

    // check for incoming uart characters if nothing received on usb
    if (gotCharacter == -1) {
      gotCharacter = main_poll_uart(&rxData);
    }

    if (gotCharacter != -1) {
      if (settingActive) {
        switch (rxData) {
        case '\n':
        case '\r':
          settingActive         = false;
          charBuf[settingCount] = '\0';

          if (charBuf[0] == 'k') {
            rgb_mgr_set_brightness(strtoul((char *)&charBuf[1], NULL, 10));
          }
          break;

        case 'r':
          settingActive     = false;
          tempColor.all     = 0;
          tempColor.color.r = rgb_mgr_get_brightness();
          rgb_mgr_set_color(&tempColor);
          break;

        case 'g':
          settingActive     = false;
          tempColor.all     = 0;
          tempColor.color.g = rgb_mgr_get_brightness();
          rgb_mgr_set_color(&tempColor);
          break;

        case 'b':
          settingActive     = false;
          tempColor.all     = 0;
          tempColor.color.b = rgb_mgr_get_brightness();
          rgb_mgr_set_color(&tempColor);
          break;

        case 'i':
          settingActive = false;
          incrementactive = !incrementactive;
          rgb_mgr_set_increment(incrementactive);
          break;

        case 's':
          settingActive = false;
          scrollactive = !scrollactive;
          rgb_mgr_set_scroll(scrollactive);
          break;

        case 't':
          settingActive = false;
          testactive = !testactive;
          rgb_mgr_set_test(testactive);
          break;

        default:
          charBuf[settingCount++] = rxData;
          break;
        }
      }
      else if (inputActive) {
        switch (rxData) {
        case '\n':
        case '\r':
          inputActive = false;
          break;

        default:

          // enter character
          if (rgb_mgr_is_printable(rxData) && (settingCount < SIZE_OF_CHARBUF)) {
            charBuf[settingCount++] = rxData;
            rgb_mgr_set_new_str(charBuf, settingCount);
          }
          break;
        }
      }
      else {
        switch (rxData) {
        case 0x11 /* ctrl-q */:
          settingActive = true;
          settingCount  = 0;
          break;

        case 0x12 /* ctrl-r */:
          inputActive  = true;
          settingCount = 0;
          break;

        default:
          break;
        }
      }
    }
  }

  return 0;
}
