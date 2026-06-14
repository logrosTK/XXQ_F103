/*
 * @文件描述:
 * @作者: Q
 * @Date: 2023-02-13 16:09:51
 * @LastEditTime: 2023-03-23 10:39:35
 */
#ifndef _APP_SENSOR_H_
#define _APP_SENSOR_H_
#include "user_main.h"

void app_sensor_init(void);
void app_sensor_run(void);
uint8_t app_sensor_refresh_ultrasonic_distance(void);
uint32_t app_sensor_get_ultrasonic_distance_x100(void);
uint8_t app_sensor_is_ultrasonic_distance_valid(void);
uint8_t app_sensor_get_tracking_state(uint8_t *buffer, uint8_t buffer_len);
uint8_t app_sensor_get_tracking_mask(void);
uint8_t app_sensor_is_tracking_online(void);
uint8_t app_sensor_get_tracking_cross_count(void);
uint8_t app_sensor_get_tracking_cross_hold(void);
uint8_t app_sensor_get_tracking_mode(void);
#endif
