
#include "MKL26Z4.h"
#include "systick.h"
#include <string.h> //memset
#include "usb_main.h"
#include <stdlib.h> // strtoul
#include <stdbool.h>
#include "font_5x7.h"

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

typedef union rgbData_u {
    uint32_t all;
    uint8_t byte[4];
    struct {
        uint8_t g;
        uint8_t r;
        uint8_t b;
        uint8_t pad;
    } color;
}rgbData_t;

//
//  encodeWS2812B - encode 8 bits into 24 bits. pOut is 3x pIn length. len is
//                  bytes.
//
static void encodeWS2812B(uint8_t *pIn, uint8_t *pOut, uint32_t len)
{
    uint8_t i,c;
    uint32_t j;
    rgbData_t output;

    for(j=0; j<len; j++){
        output.all = 0;
        c = pIn[j];
        // encode 1 byte into 3.
        for(i=0; i<8; i++){
            if(c & 0x80){
                output.all |= (0x6);
            }
            else{
                output.all |= (0x4);
            }
            c <<= 1;
            output.all <<= 3;
        }

        output.all >>= 3;

        pOut[3*j+0] = output.byte[2];
        pOut[3*j+1] = output.byte[1];
        pOut[3*j+2] = output.byte[0];
    }
}

#define RESET_BRIGHTNESS 7
// dead area, used as reset pulse in 1-wire ws2812 interface
#define SPI_DEAD_PULSE_LENGTH 260
#define SPI_WS2812_LIGHT_COUNT 84
static uint8_t spiData[SPI_DEAD_PULSE_LENGTH + 3*3*SPI_WS2812_LIGHT_COUNT];
static uint8_t *rawData;
static rgbData_t rgbData = {.color = {.g=RESET_BRIGHTNESS, .r=0x00, .b=0x00}};
static bool incrementActive = false;
static const uint8_t zeroLed[] = {0x92, 0x49, 0x24, 0x92, 0x49, 0x24, 0x92, 0x49, 0x24};
static uint8_t brightness = RESET_BRIGHTNESS;
static uint32_t dataOff = RESET_BRIGHTNESS;

static void main_increment_lights(uint8_t maxBrightness)
{
    // set the rgb values
    if(dataOff < maxBrightness){
        rgbData.color.b--;
        rgbData.color.g++;
    }
    else if (dataOff < maxBrightness * 2){
        // steady for 1 period
        asm("nop");
    }
    else if(dataOff < maxBrightness * 3){
        rgbData.color.g--;
        rgbData.color.r++;
    }
    else if (dataOff < maxBrightness * 4){
        // steady for 1 period
        asm("nop");
    }
    else if(dataOff < maxBrightness * 5){
        rgbData.color.r--;
        rgbData.color.b++;
    }
    else if (dataOff < maxBrightness * 6){
        // steady for 1 period
        asm("nop");
    }

    dataOff = (dataOff + 1) % (maxBrightness * 6);
}

static void main_toggle_increment(void)
{
    incrementActive = !incrementActive;
    if(incrementActive){
        dataOff = brightness;
        rgbData.all = 0;
        rgbData.color.r = 0;
        rgbData.color.g = brightness;
        rgbData.color.b = 0;
    }
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

//
// isPrintable  -   is printable ascii?
//
static bool isPrintable(uint8_t chr)
{
    return (chr > 31) && (chr < 127);
}

static void setPixel(uint32_t x, int y, uint8_t *pBuf, rgbData_t *pColor)
{
    uint32_t offset = (x / 12)*SPI_WS2812_LIGHT_COUNT;
    offset += y * 12;
    offset += (y & 1)?(11 - (x % 12)):(x % 12);

    encodeWS2812B(pColor->byte, pBuf + offset * 9, 3);
}

static void clearPixel(uint32_t x, uint32_t y, uint8_t *pBuf)
{
    setPixel(x, y, pBuf, (rgbData_t *)"\x00\x00\x00\x00");
}

//
// setChar  -   set character value using bitmap font5x7, at specified character
//  position, with specified color
//
#define NUMBER_OF_DIGITS 2
#define ROW_LENGTH 12
static void setChar(uint32_t charOffset,
                    uint8_t charVal,
                    uint8_t *pBuf,
                    rgbData_t *pColor)
{
    const char *font = &font5x7[(charVal - 0x20) * 5];
    uint32_t i,j;

    // set the bits based on the font value
    for(i=0; i<5; i++){
        for(j=0; j<7; j++){
            if(font[i] & 1<<j){
                setPixel((charOffset + i) % ROW_LENGTH, j, pBuf, pColor);
            }
            else{
                clearPixel((charOffset + i) % ROW_LENGTH, j, pBuf);
            }
        }
    }
    for(j=0; j<7; j++){
        clearPixel((charOffset + 5) % ROW_LENGTH, j, pBuf);
    }
}

uint8_t printChars[2] = {'1', '2'};
static void main_rgb(void)
{
    static uint32_t rgbTime = 0;
    static uint32_t scrollIdx = 0;
    static uint32_t scrollTime = 0;

    // scroll!
    if(systick_getMs() - scrollTime > 100){
        scrollTime = systick_getMs();
        setChar(scrollIdx,
                printChars[0],
                rawData,
                &rgbData);
        setChar(scrollIdx + 6,
                printChars[1],
                rawData,
                &rgbData);
        scrollIdx--;
        if(scrollIdx > 11){
            scrollIdx = 11;
        }

        if(incrementActive){
            main_increment_lights(brightness);
        }

    }

    // refresh every 50 ms
    if(systick_getMs() - rgbTime > 50){
        rgbTime = systick_getMs();
        main_spi_send(spiData, sizeof(spiData));
    }
}

int main(void)
{
    uint8_t cdcChar;
    uint8_t activeChar = 0;
    uint8_t charBuf[5];
    uint8_t settingCount;
    bool settingActive = false;
    int i;

    // initialize the necessary
    main_init_io();
    main_init_spi();

    // init buffer for spi packed data- fixed low pulse of 160 byte-times
    rawData = &spiData[SPI_DEAD_PULSE_LENGTH];
    memset(spiData, 0, sizeof(spiData));
    // zero all pixels
    for(i=0; i<SPI_WS2812_LIGHT_COUNT*9; i+=9){
        memcpy(&rawData[i], zeroLed, 9);
    }

    // usb init
    usb_main_init();

    while(1){
        // led task
        main_led();

        // rgb task
        main_rgb();

        // usb task
        if(usb_main_mainfunction(&cdcChar) != -1){
            if(settingActive){
                switch(cdcChar){
                    case '\n':
                    case '\r':
                        settingActive = false;
                        charBuf[settingCount] = '\0';
                        if(charBuf[0] == 'k'){
                            brightness = strtoul((char*)&charBuf[1], NULL, 10);
                            if(rgbData.color.r){
                                rgbData.color.r = brightness;
                            }
                            if(rgbData.color.g){
                                rgbData.color.g = brightness;
                            }
                            if(rgbData.color.b){
                                rgbData.color.b = brightness;
                            }
                        }
                        break;
                    case 'r':
                        settingActive = false;
                        rgbData.all = 0;
                        rgbData.color.r = brightness;
                        break;

                    case 'g':
                        settingActive = false;
                        rgbData.all = 0;
                        rgbData.color.g = brightness;
                        break;

                    case 'b':
                        settingActive = false;
                        rgbData.all = 0;
                        rgbData.color.b = brightness;
                        break;

                    case 'i':
                        settingActive = false;
                        main_toggle_increment();
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
                        if(isPrintable(cdcChar)){
                            printChars[activeChar++] = cdcChar;
                            if(activeChar > NUMBER_OF_DIGITS - 1){
                                activeChar = 0;
                            }
                        }
                        break;
                }
            }
        }
    }

    return 0;
}
