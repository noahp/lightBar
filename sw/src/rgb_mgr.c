
//
// rgb_mgr.c
// RGB LED manager.
//

#include "rgb_mgr.h"
#include "font_5x7.h"
#include "systick.h"
#include <string.h>

#define RESET_BRIGHTNESS 7
static rgbData_t rgbData = {.color = {.g=RESET_BRIGHTNESS, .r=0x00, .b=0x00}};

#define NUMBER_OF_DIGITS 2
#define ROW_LENGTH 12

static uint8_t *rawData;
static bool incrementActive = true;
static bool scrollActive = true;
static bool testActive = false;
static const uint8_t zeroLed[] = {0x92, 0x49, 0x24, 0x92, 0x49, 0x24, 0x92, 0x49, 0x24};
static uint8_t brightness = RESET_BRIGHTNESS;
static uint32_t dataOff = RESET_BRIGHTNESS;
static uint8_t printChars[2] = {'1', '2'};
static uint8_t activeChar = 0;

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

static void rgb_mgr_increment_lights(uint8_t maxBrightness)
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

void rgb_mgr_init(uint8_t *pBuf)
{
    int i;

    // init buffer for spi packed data- fixed low pulse of 160 byte-times
    rawData = pBuf;
    // zero all pixels
    for(i=0; i<SPI_WS2812_LIGHT_COUNT*9; i+=9){
        memcpy(&rawData[i], zeroLed, 9);
    }
}

void rgb_mgr_set_brightness(uint8_t newBrightness)
{
    brightness = newBrightness;
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

void rgb_mgr_set_color(rgbData_t *newColor)
{
    rgbData.all = newColor->all;
}

uint8_t rgb_mgr_get_brightness(void)
{
    return brightness;
}

void rgb_mgr_toggle_increment(void)
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

void rgb_mgr_toggle_scroll(void)
{
    scrollActive = !scrollActive;
}

void rgb_mgr_toggle_test(void)
{
    testActive = !testActive;
}

//
// rgb_mgr_is_printable -   is printable ascii?
//
bool rgb_mgr_is_printable(uint8_t chr)
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
void rgb_mgr_setChar(uint32_t charOffset,
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

void rgb_mgr_set_new_char(uint8_t newChar)
{
    printChars[activeChar++] = newChar;
    if(activeChar > NUMBER_OF_DIGITS - 1){
        activeChar = 0;
    }
}

void rgb_mgr_main_function(void)
{
    static uint32_t scrollIdx = 0;
    static uint32_t scrollTime = 0;
    int i;

    // print chars
    if(systick_getMs() - scrollTime > 100){
        if(testActive){
            for(i=0; i<SPI_WS2812_LIGHT_COUNT; i++){
                encodeWS2812B(rgbData.byte, rawData + i * 9, 3);
            }
        }
        else{
            scrollTime = systick_getMs();
            rgb_mgr_setChar(scrollIdx, printChars[0], rawData, &rgbData);
            rgb_mgr_setChar(scrollIdx + 6, printChars[1], rawData, &rgbData);
        }

        // scroll?
        if(scrollActive){
            scrollIdx--;
            if(scrollIdx > 11){
                scrollIdx = 11;
            }
        }

        // sweep color?
        if(incrementActive){
            rgb_mgr_increment_lights(brightness);
        }

    }
}

