/*
 * File:		usb_main.c
 * Purpose:		Main process
 *
 */

/* Includes */
#include "usb_main.h"
#include "usb_cdc.h"
#include "usb_reg.h"
#include "Settings.h"

volatile uint8_t  gu8ISR_Flags=0;

/* Extern Variables */
extern uint8_t gu8USB_Flags;
extern uint8_t gu8EP3_OUT_ODD_Buffer[];
extern tBDT tBDTtable[];


/********************************************************************/
void usb_main_init (void)
{
    PRINTF("\n********** USB Device Module Testing **********\n");

    USB_REG_SET_ENABLE;
    USB_REG_SET_STDBY_STOP;
    USB_REG_SET_STDBY_VLPx;

    #ifdef TOWER //there is no VBUS pin detection at freedom
    /* VBUS detection  */
    SIM_SCGC5|= SIM_SCGC5_PORTC_MASK;
    PORTC_PCR17 |= PORT_PCR_MUX(1);
    GPIOC_PDDR &= ~(1<<17);
    while (!(GPIOC_PDIR & (1<<17)));  //Wait until VBUS is detected
    PRINTF ("VBUS up");
    #endif

    // USB CDC Initialization
    CDC_Init();
}

int usb_main_mainfunction(uint8_t *pChar)
{
    int retval = -1;

    // USB CDC service routine
    CDC_Engine();

    // If data transfer arrives
    if(FLAG_CHK(EP_OUT,gu8USB_Flags))
    {
        (void)USB_EP_OUT_SizeCheck(EP_OUT);
        usbEP_Reset(EP_OUT);
        usbSIE_CONTROL(EP_OUT);
        FLAG_CLR(EP_OUT,gu8USB_Flags);

        // Send it back to the PC
        EP_IN_Transfer(EP2,CDC_OUTPointer,1);

        // save the value for the calling application
        *pChar = CDC_OUTPointer[0];
        retval = 1;
    }

    #ifdef TOWER
    if(!(GPIOC_PDIR & (1<<17)))
    {
        PRINTF ("\nVBUS down");
        while (!(GPIOC_PDIR & (1<<17)));  //Wait until VBUS is detected
        PRINTF ("\n\nVBUS up");
        FLAG_SET(VBUS_HIGH_EVENT,gu8ISR_Flags);
    }
    #endif

    return retval;
}

/*******************************************************************/
/*******************************************************************/
