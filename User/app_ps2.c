/*
 * ================================================================================
 * @文件名称: app_ps2.c
 * @功能描述: 应用层PS2手柄控制模块
 *           读取手柄数据，处理摇杆控制小车和按键触发动作组
 *           支持红灯模式和绿灯模式两种按键映射
 * @所属模块: User层
 * @依赖: app_ps2.h, ps2/y_ps2.h, app_motor.h
 * @数据说明:
 *   psx_buf[0]=模式, buf[1]=LED状态, buf[2]=按键高字节
 *   buf[3-4]=16按键位图, buf[5-6]=右摇杆X/Y, buf[7-8]=左摇杆X/Y
 *   绿灯模式(PS2_LED_GRN): 摇杆控制小车运动
 *   按键按下释放分别触发pre_cmd_set_red/pre_cmd_set_grn中定义的命令
 * ================================================================================
 */

#include "app_ps2.h"

/*
 * 全局数组: pre_cmd_set_red
 * 功能描述: 绿灯模式下16个按键按下(PRESS)时触发的命令字符串
 *           按键顺序: L2,R2,L1,R1,RU,RR,RD,RL,SE,AL,AR,ST,LU,LR,LD,LL
 *           格式: "<PS2_REDxx:命令^释放命令>"
 *           其中^前为按下命令，^后为释放命令
 */
const char *pre_cmd_set_red[PSX_BUTTON_NUM] = {
	"<PS2_RED01:#005P0600T2000!^#005PDST!>", // L2
	"<PS2_RED02:#005P2400T2000!^#005PDST!>", // R2
	"<PS2_RED03:#004P0600T2000!^#004PDST!>", // L1
	"<PS2_RED04:#004P2400T2000!^#004PDST!>", // R1
	"<PS2_RED05:#002P2400T2000!^#002PDST!>", // RU
	"<PS2_RED06:#003P2400T2000!^#003PDST!>", // RR
	"<PS2_RED07:#002P0600T2000!^#002PDST!>", // RD
	"<PS2_RED08:#003P0600T2000!^#003PDST!>", // RL
	"<PS2_RED09:$DJR!>",					 // SE
	"<PS2_RED10:>",							 // AL
	"<PS2_RED11:>",							 // AR
	"<PS2_RED12:$DJR!>",					 // ST
	"<PS2_RED13:#001P0600T2000!^#001PDST!>", // LU
	"<PS2_RED14:#000P0600T2000!^#000PDST!>", // LR
	"<PS2_RED15:#001P2400T2000!^#001PDST!>", // LD
	"<PS2_RED16:#000P2400T2000!^#000PDST!>", // LL
};

/*
 * 全局数组: pre_cmd_set_grn
 * 功能描述: 红灯模式下16个按键按下(PRESS)时触发的命令字符串
 *           使用$DCR命令控制小车运动（红灯模式下按键控制小车）
 */
const char *pre_cmd_set_grn[PSX_BUTTON_NUM] = {
	"<PS2_RED01:$DCR:0,500,500,0!^$DCR:0,0,0,0!>",			   // L2    左上500
	"<PS2_RED02:$DCR:500,0,0,500!^$DCR:0,0,0,0!>",			   // R2	右上500
	"<PS2_RED03:$DCR:0,1000,1000,0!^$DCR:0,0,0,0!>",		   // L1	左上1000
	"<PS2_RED04:$DCR:1000,0,0,1000!^$DCR:0,0,0,0!>",		   // R1	右上1000
	"<PS2_RED05:$DCR:1000,1000,1000,1000!^$DCR:0,0,0,0!>",	   // RU	前进1000
	"<PS2_RED06:$DCR:1000,-1000,-1000,1000!^$DCR:0,0,0,0!>",   // RR	右平移1000
	"<PS2_RED07:$DCR:-1000,-1000,-1000,-1000!^$DCR:0,0,0,0!>", // RD	后退1000
	"<PS2_RED08:$DCR:-1000,1000,1000,-1000!^$DCR:0,0,0,0!>",   // RL	左平移1000
	"<PS2_RED09:$DJR!>",									   // SE
	"<PS2_RED10:>",											   // AL
	"<PS2_RED11:>",											   // AR
	"<PS2_RED12:$DJR!>",									   // ST
	"<PS2_RED13:$DCR:500,500,500,500!^$DCR:0,0,0,0!>",		   // LU	前进500
	"<PS2_RED14:$DCR:500,-500,500,-500!^$DCR:0,0,0,0!>",	   // LR	右转500
	"<PS2_RED15:$DCR:-500,-500,-500,-500!^$DCR:0,0,0,0!>",	   // LD	后退500
	"<PS2_RED16:$DCR:-500,500,-500,500!^$DCR:0,0,0,0!>",	   // LL	左转500
};

void parse_psx_buf(unsigned char *buf, unsigned char mode);
void loop_ps2_car(void);

/*
 * 函数名称: app_ps2_init
 * 功能描述: 初始化PS2手柄硬件（GPIO引脚）
 * 参数说明: 无
 * 返回值:   无
 * 使用说明: 在系统初始化时调用一次
 */
void app_ps2_init(void)
{
	ps2_init();
}

/*
 * 函数名称: app_ps2_run
 * 功能描述: PS2手柄主循环（每50ms执行一次）
 *           读取手柄数据 -> 处理摇杆控制 -> 检测按键变化并解析
 * 参数说明: 无
 * 返回值:   无
 * 使用说明: 在主循环中调用
 *           仅当按键状态变化时才触发parse_psx_buf解析
 *           避免重复解析相同按键状态
 */
void app_ps2_run(void)
{
	static unsigned char psx_button_bak[2] = {0};
	static u32 systick_ms_bak = 0;

	if (millis() - systick_ms_bak < 50)
		return;
	systick_ms_bak = millis();

	ps2_write_read();

	loop_ps2_car();

	if ((psx_button_bak[0] == psx_buf[3]) && (psx_button_bak[1] == psx_buf[4]))
	{
	}
	else
	{
		parse_psx_buf(psx_buf + 3, psx_buf[1]);
		psx_button_bak[0] = psx_buf[3];
		psx_button_bak[1] = psx_buf[4];
//		printf("psx_button_bak[0] = %d ,psx_button_bak[1] = %d\r\n ,psx_buf[1]=%d",psx_button_bak[0],psx_button_bak[1],psx_buf[1]);
	}
}

/*
 * 函数名称: loop_ps2_car
 * 功能描述: 处理PS2摇杆控制麦轮小车（仅绿灯模式下生效）
 *           左摇杆(Y轴buf[8]): 前进/后退
 *           左摇杆(X轴buf[7]): 左移/右移
 *           右摇杆(X轴buf[5]): 左转/右转
 *           使用麦轮运动学公式将摇杆值转换为四个轮子的速度
 * 参数说明: 无
 * 返回值:   无
 * 使用说明: 绿灯模式(PS2_LED_GRN)下自动生效
 *           摇杆中值127，死区±10（通过num变量定义）
 *           速度值除以1000转换为m/s传给motor_speed_set
 */
void loop_ps2_car(void)
{

	float car_left1 = 0, car_right1 = 0, car_left2 = 0, car_right2 = 0;
	static float car_left1_bak = 0, car_right1_bak = 0, car_left2_bak = 0, car_right2_bak = 0;
	static u8 num = 10;

	if (psx_buf[1] != PS2_LED_GRN)
		return;

	if (abs(127 - psx_buf[7]) > num)
	{
		car_left1 = car_left1 - (0x7f - psx_buf[7]) * 2;
		car_right1 = car_right1 + (0x7f - psx_buf[7]) * 2;
		car_left2 = car_left2 - (0x7f - psx_buf[7]) * 2;
		car_right2 = car_right2 + (0x7f - psx_buf[7]) * 2;
	}
	if (abs(127 - psx_buf[8]) > num)
	{
		car_left1 = car_left1 + (0x7f - psx_buf[8]) * 2;
		car_right1 = car_right1 + (0x7f - psx_buf[8]) * 2;
		car_left2 = car_left2 + (0x7f - psx_buf[8]) * 2;
		car_right2 = car_right2 + (0x7f - psx_buf[8]) * 2;
	}
	if (abs(127 - psx_buf[5]) > num)
	{
		car_left1 = car_left1 - (0x7f - psx_buf[5]) * 2;
		car_right1 = car_right1 + (0x7f - psx_buf[5]) * 2;
		car_left2 = car_left2 + (0x7f - psx_buf[5]) * 2;
		car_right2 = car_right2 - (0x7f - psx_buf[5]) * 2;
	}
	if (abs(127 - psx_buf[6]) > num)
	{
		car_left1 = car_left1 + (0x7f - psx_buf[6]) * 2;
		car_right1 = car_right1 + (0x7f - psx_buf[6]) * 2;
		car_left2 = car_left2 + (0x7f - psx_buf[6]) * 2;
		car_right2 = car_right2 + (0x7f - psx_buf[6]) * 2;
	}
//	printf("pss %d  %d  %d  %d\n",abs(127 - psx_buf[7]),abs(127 - psx_buf[8]),abs(127 - psx_buf[5]),abs(127 - psx_buf[6]));
	if ((car_left1_bak != car_left1) || (car_right1_bak != car_right1) || (car_left2_bak != car_left2) || (car_right2_bak != car_right2))
	{
		
	//		printf("pss %d  %d  %d  %d\n",abs(127 - psx_buf[7]),abs(127 - psx_buf[8]),abs(127 - psx_buf[5]),abs(127 - psx_buf[6]));
//		printf("car_left1 %f ,car_right1 %f ,car_left2 %f ,car_right2 %f ",car_left1,car_right1,car_left2,car_right2);
		motor_speed_set(car_left1 / 1000, car_right1 / 1000, car_left2 / 1000, car_right2 / 1000);
	}
	car_left1_bak = car_left1;
	car_right1_bak = car_right1;

	car_left2_bak = car_left2;
	car_right2_bak = car_right2;

}

/*
 * 函数名称: parse_psx_buf
 * 功能描述: 解析PS2手柄按键缓冲区，根据按键变化触发对应命令
 *           检测16个按键的按下(press)和释放(release)事件
 *           按键按下: 触发pre_cmd_set_red/grn中^前的命令
 *           按键释放: 触发pre_cmd_set_red/grn中^后的命令
 * 参数说明: buf - 按键数据缓冲区指针（psx_buf+3，2字节16按键位图）
 *          mode - 手柄模式 (PS2_LED_GRN绿灯/PS2_LED_RED红灯)
 * 返回值:   无
 * 使用说明: 由app_ps2_run()在检测到按键变化时调用
 *           命令以$开头调用parse_cmd(), 以#开头调用parse_action()
 *           使用bak变量保存上一次按键状态以检测变化
 */
void parse_psx_buf(unsigned char *buf, unsigned char mode)
{
	u8 i;
	uint16_t pos = 0;
	static u16 bak = 0xffff, temp, temp2;
	temp = (buf[0] << 8) + buf[1];

	if (bak != temp)
	{
		temp2 = temp;
		temp &= bak;
		for (i = 0; i < 16; i++)
		{
			if ((1 << i) & temp)
			{
			}
			else
			{
				if ((1 << i) & bak)
				{
					// press 表示按键按下了
					memset(cmd_return, 0, sizeof(cmd_return));
					if (mode == PS2_LED_GRN)
					{
						memcpy(cmd_return, pre_cmd_set_red[i], strlen(pre_cmd_set_red[i]));
					}
					else if (mode == PS2_LED_RED)
					{
						memcpy(cmd_return, pre_cmd_set_grn[i], strlen(pre_cmd_set_grn[i]));
					}
					else
						continue;
					// zx_uart_send_str(cmd_return);
					pos = str_contain_str(cmd_return, "^");
					if (pos)
						cmd_return[pos - 1] = '\0';
					if (str_contain_str(cmd_return, "$"))
					{
						parse_cmd(cmd_return + 11);
					}
					else if (str_contain_str(cmd_return, "#"))
					{
						parse_action(cmd_return + 11);
					}
					bak = 0xffff;
				}
				else
				{ // release 表示按键松开了

					memset(cmd_return, 0, sizeof(cmd_return));
					if (mode == PS2_LED_GRN)
					{
						memcpy(cmd_return, pre_cmd_set_red[i], strlen(pre_cmd_set_red[i]));
					}
					else if (mode == PS2_LED_RED)
					{
						memcpy(cmd_return, pre_cmd_set_grn[i], strlen(pre_cmd_set_grn[i]));
					}
					else
						continue;

					pos = str_contain_str(cmd_return, "^");
					if (pos)
					{
						if (str_contain_str(cmd_return + pos, "$"))
						{
							cmd_return[strlen(cmd_return) - 1] = '\0';
							parse_cmd(cmd_return + pos);
						}
						else if (str_contain_str(cmd_return + pos, "#"))
						{
							cmd_return[strlen(cmd_return) - 1] = '\0';
							parse_action(cmd_return + pos);
						}
					}
				}
			}
		}
		bak = temp2;
	}
	return;
}
