
#include "MKL26Z4.h"
#include "systick.h"
#include <string.h> //memset
#include "usb_main.h"
#include <stdlib.h> // strtoul
#include <stdbool.h>
#include "rgb_mgr.h"

// dead area, used as reset pulse in 1-wire ws2812 interface
#define SPI_DEAD_PULSE_LENGTH 260
static uint8_t spiData[SPI_DEAD_PULSE_LENGTH + 3*3*SPI_WS2812_LIGHT_COUNT];

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

//static void delay_ms(uint32_t ms)
//{
//    uint32_t start = systick_getMs();
//    while(systick_getMs() - start < ms);
//}

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
    UART0_C4 = UART0_C4_OSR(25-1);

    // set baud rate to 9600
    baud = (48000000 / 9600) / 25;  // <- baud = CLOCK / ((OSR+1)*BR)
    UART0_BDH = UART0_BDH_SBR((baud >> 8) & 0x1F);  // upper 5 bits
    UART0_BDL = UART0_BDL_SBR(baud & 0xFF);         // lower 8 bits

    // turn on tx + rx
    UART0_C2 = UART0_C2_RE_MASK | UART0_C2_TE_MASK;

    // send a char?
    UART0_D = 'N';
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
    DMAMUX0_BASE_PTR->CHCFG[0] &= ~(DMAMUX_CHCFG_ENBL_MASK | DMAMUX_CHCFG_SOURCE_MASK);
    // select channel 17 for spi tx & enable it
    DMAMUX0_BASE_PTR->CHCFG[0] |= DMAMUX_CHCFG_SOURCE(17) | DMAMUX_CHCFG_ENBL_MASK;

    // dma
    // set destination address register to the SPI tx register
    DMA_BASE_PTR->DMA[0].DAR = (uint32_t)&SPI0_DL;
    // configure DMA_0 for spi tx
    DMA_BASE_PTR->DMA[0].DCR = DMA_DCR_ERQ_MASK |       // enable periph. request
                               DMA_DCR_CS_MASK |
                               DMA_DCR_SINC_MASK |
                               DMA_DCR_SSIZE(0x01) |
                               DMA_DCR_DSIZE(0x01) |
                               DMA_DCR_D_REQ_MASK;
}

static void main_spi_send(uint8_t *pData, uint32_t len)
{
    // wait until dma busy flag is cleared
    while(DMA_BASE_PTR->DMA[0].DSR_BCR & DMA_DSR_BCR_BSY_MASK);
//    // pull spi mosi low (p27) for at least 50us
//    PORTC_PCR6 = PORT_PCR_MUX(1);
//    GPIOC_PCOR = (1 << 6);
//    delay_ms(2);
    // reset dma
    DMA_BASE_PTR->DMA[0].DSR_BCR = DMA_DSR_BCR_DONE_MASK;
    // write to the dma
    DMA_BASE_PTR->DMA[0].SAR = (uint32_t)pData;
    DMA_BASE_PTR->DMA[0].DSR_BCR = len & 0x00ffffff;
    // enable dma on the spi
    DMA_BASE_PTR->DMA[0].DCR |= DMA_DCR_ERQ_MASK;
    //PORTC_PCR6 = PORT_PCR_MUX(2);
    SPI0_C2 |= SPI_C2_TXDMAE_MASK;
//    // wait until dma busy flag is cleared
//    while(DMA_BASE_PTR->DMA[0].DSR_BCR & DMA_DSR_BCR_BSY_MASK);
}

static void main_led(void)
{
    static uint32_t blinkTime = 0;

    // blink every 250ms
    if(systick_getMs() - blinkTime > 250){
        blinkTime = systick_getMs();
        // toggle
        GPIOB_PTOR = (1 << 0);
    }
}

int main(void)
{
    static uint32_t rgbTime = 0;
    uint8_t cdcChar;
    uint8_t charBuf[5];
    uint8_t settingCount;
    bool settingActive = false;
    rgbData_t tempColor;

    // initialize the necessary
    main_init_io();
    main_init_spi();
    main_init_uart();

    // rgb init
    memset(spiData, 0, sizeof(spiData));
    rgb_mgr_init(&spiData[SPI_DEAD_PULSE_LENGTH]);

    // usb init
    usb_main_init();

    while(1){
        // led task
        main_led();

        // rgb task
        rgb_mgr_main_function();

        // refresh every 50 ms
        if(systick_getMs() - rgbTime > 50){
            rgbTime = systick_getMs();
            main_spi_send(spiData, sizeof(spiData));
        }

        // usb task
        if(usb_main_mainfunction(&cdcChar) != -1){
            if(settingActive){
                switch(cdcChar){
                    case '\n':
                    case '\r':
                        settingActive = false;
                        charBuf[settingCount] = '\0';
                        if(charBuf[0] == 'k'){
                            rgb_mgr_set_brightness(strtoul((char*)&charBuf[1], NULL, 10));
                        }
                        break;
                    case 'r':
                        settingActive = false;
                        tempColor.all = 0;
                        tempColor.color.r = rgb_mgr_get_brightness();
                        rgb_mgr_set_color(&tempColor);
                        break;

                    case 'g':
                        settingActive = false;
                        tempColor.all = 0;
                        tempColor.color.g = rgb_mgr_get_brightness();
                        rgb_mgr_set_color(&tempColor);
                        break;

                    case 'b':
                        settingActive = false;
                        tempColor.all = 0;
                        tempColor.color.b = rgb_mgr_get_brightness();
                        rgb_mgr_set_color(&tempColor);
                        break;

                    case 'i':
                        settingActive = false;
                        rgb_mgr_toggle_increment();
                        break;

                    case 's':
                        settingActive = false;
                        rgb_mgr_toggle_scroll();
                        break;

                    case 't':
                        settingActive = false;
                        rgb_mgr_toggle_test();
                        break;

                    default:
                        charBuf[settingCount++] = cdcChar;
                        break;
                }
            }
            else{
                switch(cdcChar){
                    case 0x11 /* ctrl-q */:
                        settingActive = true;
                        settingCount = 0;
                        break;

                    default:
                        // enter character
                        if(rgb_mgr_is_printable(cdcChar)){
                            rgb_mgr_set_new_char(cdcChar);
                        }
                        break;
                }
            }
        }
    }

    return 0;
}
