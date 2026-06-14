/*
 * ================================================================================
 * @文件名称: tracking.c
 * @功能描述: 4路红外循迹传感器驱动，初始化传感器GPIO引脚为浮空输入模式
 * @所属模块: Components/tracking
 * @依赖: tracking.h
 * @硬件说明: 4路红外探头分别对应X1/X2/X3/X4，用于检测地面黑线
 *           X1: 最右侧, X2: 中间右, X3: 中间左, X4: 最左侧
 * ================================================================================
 */

#include "tracking/tracking.h"


/*
 * 函数名称: TRACK_IR4_Init
 * 功能描述: 初始化4路红外循迹传感器GPIO引脚，配置为浮空输入模式
 * 参数说明: 无
 * 返回值:   无
 * 使用说明: 在传感器初始化阶段调用一次
 *           初始化后可通过以下宏读取各探头状态:
 *           TRTACK_IR4_X1_READ() - 读取X1探头（最右侧）
 *           TRTACK_IR4_X2_READ() - 读取X2探头（中间右）
 *           TRTACK_IR4_X3_READ() - 读取X3探头（中间左）
 *           TRTACK_IR4_X4_READ() - 读取X4探头（最左侧）
 *           返回0表示检测到黑线，1表示未检测到（白线/地面）
 */
void TRACK_IR4_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

    RCC_APB2PeriphClockCmd(TRTACK_IR4_X1_CLK | TRTACK_IR4_X2_CLK | TRTACK_IR4_X3_CLK | TRTACK_IR4_X4_CLK, ENABLE); 
	
	GPIO_InitStructure.GPIO_Pin = TRTACK_IR4_X1_PIN;         /* 配置 pin */
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;  	/* 浮空输入 */
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz; 		 /* IO 翻转 50MHz */
    GPIO_Init(TRTACK_IR4_X1_PORT, &GPIO_InitStructure);
	
    GPIO_InitStructure.GPIO_Pin = TRTACK_IR4_X2_PIN;
    GPIO_Init(TRTACK_IR4_X2_PORT, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = TRTACK_IR4_X3_PIN;
    GPIO_Init(TRTACK_IR4_X3_PORT, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = TRTACK_IR4_X4_PIN;
    GPIO_Init(TRTACK_IR4_X4_PORT, &GPIO_InitStructure);
}


