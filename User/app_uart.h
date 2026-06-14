#ifndef _APP_UART_H_
#define _APP_UART_H_
#include "user_main.h"

void app_uart_init(void);
void app_uart_run(void);
void app_uart_print_help(void);
void app_uart_print_status(void);
void app_uart_print_pid(void);
void app_uart_print_ultrasonic_status(void);
void app_uart_print_tracking_status(void);
void app_uart_print_log_config(void);

void app_uart_set_motor_log(uint8_t enabled, uint32_t period_ms);
uint8_t app_uart_is_motor_log_enabled(void);
uint32_t app_uart_get_motor_log_period_ms(void);

void app_uart_set_ultrasonic_log(uint8_t enabled, uint32_t period_ms);
uint8_t app_uart_is_ultrasonic_log_enabled(void);
uint32_t app_uart_get_ultrasonic_log_period_ms(void);

#endif
