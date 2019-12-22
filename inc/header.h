


#ifndef HEADER_H_
#define HEADER_H_


#include <msp430.h>
#include <stdlib.h>
#include <stdint.h>


#define TA0_CCR0_INT 53
#define TA0_CCRN_INT 52
#define TA1_CCR0_INT 49
#define TA1_CCRN_INT 48

#define TA2_CCR0_INT 44
#define TA2_CCRN_INT 43



void Config_chaves_leds(void);


int ConfigUARTModule0(uint8_t, uint16_t, uint8_t, uint8_t);
int Debounce(void);
int DelayMicrosseconds(uint16_t);
int Delay40Microsseconds(uint16_t);
int DelaySeconds(uint8_t);

int UARTM0SendString(char*, uint16_t);

#endif // HEADER_H_
