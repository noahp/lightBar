#ifndef __CircBuffer__
#define __CircBuffer__

/* Includes */
#include "arm_cm0.h"
#include "MKL26Z4.h"

/* Error Codes */
#define     OK                  0
#define     NOT_ENOUGH_SPACE    1


/* Extern variables */
volatile extern uint8_t *OUT_StartAddress;
volatile extern uint8_t *OUT_EndAddress;
volatile extern uint8_t *OUT_UsbPointer;
volatile extern uint8_t *OUT_SciPointer;
volatile extern uint8_t gu8BufferOverFlow;


/* Prototypes */
void Buffer_Init(uint8_t* ,uint8_t);
uint8_t Buffer_Request(uint8_t* ,uint16_t);


#endif /* __CircBuffer__*/
