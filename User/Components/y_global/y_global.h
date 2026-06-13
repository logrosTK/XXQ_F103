#ifndef _Y_GLOBAL_H_
#define _Y_GLOBAL_H_

#include "user_main.h"
#include "app_motor.h"
/*
	宏定义数据
*/
#define VERSION 20  // 版本定义
#define FLAG_VERIFY 0x25  // 校验标志
#define ACTION_SIZE 512	  // 一个动作的存储大小

#define SERVO_NUM 8 /* 舵机数量 */


#define W25Q64_INFO_ADDR_SAVE_STR (((8 << 10) - 4) << 10) //(8*1024-4)*1024		//eeprom_info结构体存储的位置

extern u8 AI_mode;
extern u8 forbid_turn;//循迹模式适配地图
extern u8 group_do_ok;

extern uint8_t uart_receive_num;
extern int oled_mode;

#define CMD_RETURN_SIZE 512

#define PRE_CMD_SIZE 128

typedef struct
{
	u32 version;
	char pre_cmd[PRE_CMD_SIZE + 1];
	int dj_bias_pwm[SERVO_NUM + 1];
} eeprom_info_t;

extern eeprom_info_t eeprom_info;
extern char cmd_return[CMD_RETURN_SIZE];

uint16_t str_contain_str(char *str, char *str2);
void replace_char(char *str, char ch1, char ch2);

void parse_action(char *str);
void parse_cmd(char *cmd);

int kinematics_move(float x, float y, float z, int time);
void set_servo(int index, int pwm, int time);
void zx_uart_send_str(char *str);

void soft_reset(void);
void rewrite_eeprom(void);
void save_action(char *str);
void app_action_run(void);
void do_group_once(int group_num);
#endif
