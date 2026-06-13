#ifndef __ULTRASONIC_H
#define __ULTRASONIC_H
#include "user_main.h"


uint8_t ultrasonic_Init(void);
uint8_t ultrasonic_rgb_r(uint8_t r,uint8_t g,uint8_t b);
uint8_t ultrasonic_rgb_t(uint8_t r,uint8_t g,uint8_t b);
uint8_t ultrasonic_start_measuring(void);
float ultrasonic_read_distance(void);
#endif
