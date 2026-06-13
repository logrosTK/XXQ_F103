#ifndef _APP_PS2_H_
#define _APP_PS2_H_
#include "main.h"

#define PS2_LED_RED 0x41  // PS2手柄红灯模式
#define PS2_LED_GRN 0x73  // PS2手柄绿灯模式
#define PSX_BUTTON_NUM 16 // 手柄按键数目

void app_ps2_init(void);
void app_ps2_run(void);
#endif
