#ifndef _ENCODER_H_
#define _ENCODER_H_

#include <driver/gpio.h>

#define EC_A_PIN_NUM GPIO_NUM_0
#define EC_B_PIN_NUM GPIO_NUM_2

void init_ec(void);

#endif
