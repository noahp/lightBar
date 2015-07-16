# lightBar
Your basic WS2812B RGB LED banner. Comprised of 12x7 modules.

# hardware
Ordered the boards from dirtypcbs.com, $25 shipped. I received 11 boards (1 bonus!), 22 days turn, pretty nice.  
## schematic
[pdf view](https://github.com/noahp/lightBar/raw/master/hw/lightBar.pdf)

## pcb renders
<img src="https://github.com/noahp/lightBar/raw/master/hw/pcbfront.png" alt="pcb front">  
<img src="https://github.com/noahp/lightBar/raw/master/hw/pcbback.png" alt="pcb front">  

# software
My plan is to use some Cortex-M processor on the strip itself to do the real-time driving of the LEDs (framebuffer + SPI transmission). Probably an ESP8266 (ESP-12 module) will be used as the interface for setting the displayed data.  
Some ideas-  
- get internet time + date and display it
- display weather, possibly using a gradient over the text background showing temperature/precip conditions

I'm thinking some simple markup language (maybe JSON format) to transmit the content from the ESP8266 to the LED controller. Something that can describe text content and position, bitmaps, maybe transparency. The LED controller would parse the markup into its bitmapped frame buffer and print it on the display.
