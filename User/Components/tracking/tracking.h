/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __Y_TRACKING_H
#define __Y_TRACKING_H

#include <stdint.h>

#define TRACKING_CHANNEL_COUNT 8U

void tracking_init(void);
void tracking_run(void);
void tracking_uart_rx_byte(uint8_t rx_byte);
uint8_t tracking_is_data_valid(void);
uint8_t tracking_is_online(void);
uint8_t tracking_copy_digital(uint8_t *buffer, uint8_t buffer_len);
uint8_t tracking_get_digital_mask(void);

#endif

