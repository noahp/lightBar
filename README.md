# lightBar
Basic serial addressable WS2812B RGB LED banner. Comprised of multiple 12x7 modules.

# hardware
Ordered the boards from dirtypcbs.com, $25 shipped. I received 11 boards (1 bonus!), 22 days turn, pretty nice.  
The basic module contains 84 LEDs arranged in 7 rows of 12 each, and is designed to display 2 monospaced 5x7 pixel characters.  
Several 2-character modules can be connected together to create the full display.  

## schematic
[pdf view](https://github.com/noahp/lightBar/raw/master/hw/lightBar.pdf)

## pcb renders
<img src="https://github.com/noahp/lightBar/raw/master/hw/pcbfront.png" alt="pcb front" width="300">
<img src="https://github.com/noahp/lightBar/raw/master/hw/pcbback.png" alt="pcb back" width="300">  

# software
My plan is to use some Cortex-M processor on the strip itself to do the real-time LED control (framebuffer + SPI transmission). Probably an ESP8266 wifi module (ESP-12) will be used as the interface for setting the displayed data.  
Some ideas-  
- get internet time/date and display it
- display weather, possibly using a gradient over the text background showing temperature/precip conditions

I'm thinking some simple description language (maybe JSON format) to transmit the content from the ESP8266 to the LED controller. Something that can describe text content and position, bitmaps, maybe transparency. The LED controller would parse this data into its bitmapped frame buffer and print it on the display.
