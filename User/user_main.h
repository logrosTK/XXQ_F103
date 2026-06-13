#ifndef USER_MAIN_H
#define USER_MAIN_H

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "stm32f10x_conf.h"
#include "stm32f10x.h"
#include "adc.h"
#include "gpio.h"
#include "spi.h"
#include "tim.h"
#include "usart.h"

#include "y_delay/y_delay.h"
#include "y_timer/y_timer.h"
#include "y_led/y_led.h"
#include "y_usart/y_usart.h"
#include "y_servo/y_servo.h"
#include "y_motor/y_motor.h"
#include "y_encoder/y_encoder.h"
#include "ps2/y_ps2.h"
#include "tracking/tracking.h"
#include "flash/y_flash.h"
#include "soft_i2c/y_soft_i2c.h"
#include "y_ADC/y_ADC.h"
#include "y_beep/y_beep.h"
#include "key/key.h"
#include "y_kinematics/y_kinematics.h"
#include "y_global/y_global.h"
#include "oled/y_oled.h"
#include "ultrasonic/ultrasonic.h"

#include "app_ps2.h"
#include "app_motor.h"
#include "app_uart.h"
#include "app_sensor.h"
#include "resource/resource.h"

#ifndef bool
typedef u8 bool;
#endif

#endif
