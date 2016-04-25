
//
// rgb_mgr.c
// RGB LED manager.
//

#include "rgb_mgr.h"
#include "font_5x7.h"
#include "systick.h"
#include <string.h>
#include <stdlib.h>

#define RESET_BRIGHTNESS 7
static rgbData_t rgbData = {.color = {.g=RESET_BRIGHTNESS, .r=0x00, .b=0x00}};

#define ROW_LENGTH 12*RGB_MGR_PANEL_COUNT

static uint8_t *rawData;
static uint32_t rawDataLen = 0;
static bool incrementActive = true;
static bool scrollActive = true;
static bool testActive = false;//true;
static const uint8_t zeroLed[] = {0x92, 0x49, 0x24, 0x92, 0x49, 0x24, 0x92, 0x49, 0x24};
static uint8_t brightness = RESET_BRIGHTNESS;
static uint32_t dataOff = RESET_BRIGHTNESS;
static uint8_t *printChars;
static uint32_t printLength;

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

        pOut[3*j+0] = output.bytes[2];
        pOut[3*j+1] = output.bytes[1];
        pOut[3*j+2] = output.bytes[0];
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

static void clear_display(void)
{
    int i;

    // zero all pixels
    for(i=0; i<RGB_MGR_PANEL_COUNT*(RGB_MGR_LEDS_PER_PANEL)*9; i+=9){
        memcpy(&rawData[i], zeroLed, 9);
    }

}

void rgb_mgr_init(uint8_t *pBuf, uint32_t bufSize)
{

    // init buffer for spi packed data- fixed low pulse of 160 byte-times
    rawData = pBuf;
    rawDataLen = bufSize;

    clear_display();

    // set starting string
    rgb_mgr_set_new_str((uint8_t*)"Happy Halloween!  ", sizeof("Happy Halloween!  ") -1);
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

void rgb_mgr_set_increment(bool enable)
{
    incrementActive = enable;
    if(incrementActive){
        dataOff = brightness;
        rgbData.all = 0;
        rgbData.color.r = 0;
        rgbData.color.g = brightness;
        rgbData.color.b = 0;
    }
}

void rgb_mgr_set_scroll(bool enable)
{
    scrollActive = enable;
}

void rgb_mgr_set_test(bool enable)
{
    testActive = enable;
}

//
// rgb_mgr_is_printable -   is printable ascii?
//
bool rgb_mgr_is_printable(uint8_t chr)
{
    return (chr > 31) && (chr < 127);
}

static void setPixel(uint32_t x, int y, uint8_t *pBuf, uint32_t bufSize, rgbData_t *pColor)
{
    uint32_t offset;

    offset = (x / 12)*(RGB_MGR_LEDS_PER_PANEL);
    offset += y * 12;
    offset += (y & 1)?(11 - (x % 12)):(x % 12);

    if(offset * 9 + 3 < bufSize){
        encodeWS2812B(pColor->bytes, pBuf + offset * 9, 3);
    }
}

static void clearPixel(uint32_t x, uint32_t y, uint8_t *pBuf, uint32_t bufSize)
{
    setPixel(x, y, pBuf, bufSize, (rgbData_t *)"\x00\x00\x00\x00");
}

//
// setChar  -   set character value using bitmap font5x7, at specified character
//  position, with specified color. truncates on buffer overflow, set buffer size
//
void rgb_mgr_setChar(int charOffset,
                     uint8_t charVal,
                     uint8_t *pBuf,
                     uint32_t bufSize,
                     rgbData_t *pColor)
{
    const char *font = &font5x7[(charVal - 0x20) * 5];
    uint32_t i,j;

    // set the bits based on the font value
    for(i=0; i<6; i++){
        for(j=0; j<7; j++){
            if(charOffset + i < 0){
                break;
            }
            if(i > 4){
                clearPixel((charOffset + i), j, pBuf, bufSize);
            }
            else if(font[i] & 1<<j){
                setPixel((charOffset + i), j, pBuf, bufSize, pColor);
            }
            else{
                clearPixel((charOffset + i), j, pBuf, bufSize);
            }
        }
    }
}

void rgb_mgr_set_new_str(uint8_t *newStr, uint32_t len)
{
    printChars = (uint8_t *)newStr;
    printLength = len;
}

void rgb_mgr_main_function(void)
{
    static int scrollIdx = 0;//ROW_LENGTH;
    static uint32_t scrollTime = 0;
    int i;
//    static uint32_t xPos = 0;

    // print chars
    if(systick_getms() - scrollTime > 100){
        if(testActive){
            scrollTime = systick_getms() - 25;
//            clear_display();
//            setPixel(xPos%(12*RGB_MGR_PANEL_COUNT), xPos/(12*RGB_MGR_PANEL_COUNT), rawData, &rgbData);
//            xPos = (xPos + 1)%(RGB_MGR_PANEL_COUNT*RGB_MGR_LEDS_PER_PANEL);
            for(i=0; i<RGB_MGR_PANEL_COUNT*RGB_MGR_LEDS_PER_PANEL; i++){
                encodeWS2812B(rgbData.bytes, rawData + i * 9, 3);
            }
        }
        else{
            scrollTime = systick_getms();

            scrollIdx = 5;

            // print the string at the current offset
            for(i=0; i<2; i++){
                rgb_mgr_setChar(scrollIdx+i*6, printChars[i], rawData, rawDataLen, &rgbData);
            }
            for(; i<printLength; i++){
                rgb_mgr_setChar(scrollIdx+i*6+2, printChars[i], rawData, rawDataLen, &rgbData);
            }

            // separator
            // 12x2 12x4
            setPixel(scrollIdx + 12, 2, rawData, rawDataLen, &rgbData);
            setPixel(scrollIdx + 12, 4, rawData, rawDataLen, &rgbData);
        }

        // scroll?
        if(scrollActive){
            scrollIdx--;
            if(abs(scrollIdx) > printLength*6){
                scrollIdx = ROW_LENGTH;
            }
        }

        // sweep color?
        if(incrementActive){
            rgb_mgr_increment_lights(brightness);
        }
    }
}
