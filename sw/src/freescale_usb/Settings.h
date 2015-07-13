#define FLAG_SET(BitNumber, Register)        (Register |=(1U<<BitNumber))
#define FLAG_CLR(BitNumber, Register)        (Register &=~(1U<<BitNumber))
#define FLAG_CHK(BitNumber, Register)        (Register & (1U<<BitNumber))

#define _USB     0

//#define PRINTF(...) printf(__VA_ARGS__)
#define PRINTF(...)
