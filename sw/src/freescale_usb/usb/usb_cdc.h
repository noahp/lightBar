#ifndef __CDC__
#define __CDC__

/* Includes */
#include "arm_cm0.h"
#include "usb.h"
#include "ring_buffer.h"

extern uint8_t u8CDCState;

/** Re-enumeration macros and flag*/
extern volatile uint8_t  gu8ISR_Flags;
#define VBUS_LOW_EVENT          0x10
#define VBUS_HIGH_EVENT         0x01

//#define CUSTOM_BASE_ADDRESS 0xC000  /* Base Address for Customizable settings */


#define CDC_INPointer   gu8EP2_IN_ODD_Buffer
#define CDC_OUTPointer  gu8EP3_OUT_ODD_Buffer

#define LoaderCounter CDC_OUT_Data[CDC_BUFFER_SIZE-1]


/* Defines */
#define CDC_BUFFER_SIZE 128
#define EP_OUT          3
#define EP_IN           2   



/* USB_CDC Definitions */
#define WAITING_FOR_ENUMERATION 0x00
#define SET_LINE_CODING         0x20
#define GET_LINE_CODING         0x21
#define SET_CONTROL_LINE_STATE  0x22
#define LOADER_MODE             0xAA
#define GET_INTERFACE           0x0A
#define GET_STATUS              0x00
#define CLEAR_FEATURE           0x01
#define SET_FEATURE             0x03

/* TypeDefs */
typedef struct
{
    uint32_t  DTERate;
    uint8_t   CharFormat;
    uint8_t   ParityType;
    uint8_t   Databits;
}CDC_Line_Coding;


/* Extern variables */
extern uint8_t CDC_OUT_Data[];       
//extern uint8_t u8RecDataFlag;        
extern CDC_Line_Coding LineCoding; 


/* Prototypes */
void CDC_Init(void);
void CDC_Engine(void);
uint8_t CDC_InterfaceReq_Handler(void);
uint32_t LWordSwap(uint32_t);
uint8_t CDC_Settings_Update(uint8_t*,uint8_t);


#endif /* __CDC__*/