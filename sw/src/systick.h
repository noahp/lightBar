
//
// systick.h
// Header file for systick. Systick provides ms time
// for use by other modules.
//

#include <stdint.h>

#if !defined(SYSTICK_H)
#define SYSTICK_H

// call every ms
void systick_update(void);

// set new ms time
void systick_setms(uint32_t ms);

// return ms time
uint32_t systick_getms(void);

#endif // SYSTICK_H
