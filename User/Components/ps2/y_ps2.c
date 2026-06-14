/*
 * ================================================================================
 * @文件名称: y_ps2.c
 * @功能描述: PS2手柄底层驱动，初始化通信引脚，实现手柄数据读写
 * @所属模块: Components/ps2
 * @依赖: ps2/y_ps2.h, y_delay/y_delay.h
 * @通信协议: SPI-like协议，CLK/CMD/DAT/CS四线制
 * @数据格式: psx_buf[9] - 9字节手柄数据缓冲区
 *           buf[0]=模式, buf[1]=LED状态, buf[2]=按键状态高字节
 *           buf[3-4]=16按键位图, buf[5-8]=摇杆X/Y值
 * ================================================================================
 */

#include "ps2/y_ps2.h"

/*
 * 全局变量: psx_buf[9]
 * 功能描述: PS2手柄数据缓冲区，存储9字节通信数据
 * 数据说明: 
 *   [0] 模式 (0x41=红灯模式, 0x73=绿灯模式)
 *   [1] LED状态
 *   [2] 按键状态高字节
 *   [3-4] 16按键位图 (每个bit对应一个按键)
 *   [5] 右摇杆X轴 (0~255, 中值127)
 *   [6] 右摇杆Y轴 (0~255, 中值127)
 *   [7] 左摇杆X轴 (0~255, 中值127)
 *   [8] 左摇杆Y轴 (0~255, 中值127)
 */
u8 psx_buf[9] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};


/*
 * 函数名称: ps2_init
 * 功能描述: 初始化PS2手柄通信引脚
 *           配置DAT为下拉输入，CMD/CS/CLK为推挽输出
 *           初始化后CS和CLK置高
 * 参数说明: 无
 * 返回值:   无
 * 使用说明: 在系统初始化时调用一次
 *           初始化后可通过ps2_write_read()读取手柄数据
 */
void ps2_init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    RCC_APB2PeriphClockCmd(PS2_DAT_GPIO_CLK | PS2_CMD_GPIO_CLK | PS2_CS_GPIO_CLK | PS2_CLK_GPIO_CLK, ENABLE); // 使能端口时钟

    GPIO_InitStructure.GPIO_Pin = PS2_DAT_PIN;         // 端口配置
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPD;      // 下拉输入
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;  // 50M
    GPIO_Init(PS2_DAT_GPIO_PORT, &GPIO_InitStructure); // 根据设定参数初始化GPIO

    GPIO_InitStructure.GPIO_Pin = PS2_CMD_PIN;         // 端口配置
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;   // 推挽输出
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;  // 50M
    GPIO_Init(PS2_CMD_GPIO_PORT, &GPIO_InitStructure); // 根据设定参数初始化GPIO

    GPIO_InitStructure.GPIO_Pin = PS2_CS_PIN;         // 端口配置
    GPIO_Init(PS2_CS_GPIO_PORT, &GPIO_InitStructure); // 根据设定参数初始化GPIO

    GPIO_InitStructure.GPIO_Pin = PS2_CLK_PIN;         // 端口配置
    GPIO_Init(PS2_CLK_GPIO_PORT, &GPIO_InitStructure); // 根据设定参数初始化GPIO

    PS2_CS(1);
    PS2_CLK(1);
    PS2_CMD(1);
}

/*
 * 函数名称: ps2_transfer
 * 功能描述: PS2单字节收发函数，发送一个字节的同时接收一个字节
 * 参数说明: dat - 要发送的字节数据
 * 返回值:   接收到的字节数据
 * 使用说明: 每个CLK周期的上升沿发送1bit，下降沿读取1bit
 *           每次传输8个bit，从低位到高位
 */
u8 ps2_transfer(unsigned char dat)
{
    unsigned char rd_data, wt_data, i;
    wt_data = dat;
    rd_data = 0;
    for (i = 0; i < 8; i++)
    {
        PS2_CMD((wt_data & (0x01 << i)));
        PS2_CLK(1);
        delay_us(10);
        PS2_CLK(0);
        delay_us(10);
        PS2_CLK(1);
        if (PS2_DAT())
        {
            rd_data |= 0x01 << i;
        }
    }
    return rd_data;
}

/*
 * 函数名称: ps2_write_read
 * 功能描述: 完整读取一次PS2手柄数据（9字节），存入全局数组psx_buf
 * 参数说明: 无
 * 返回值:   无
 * 使用说明: 拉低CS片选 -> 发送START_CMD开始命令 -> 发送ASK_DAT_CMD请求数据
 *           -> 连续读取7字节数据 -> 拉高CS片选
 *           调用后psx_buf数组被更新为最新手柄数据
 *           建议每50ms调用一次
 */
void ps2_write_read(void)
{
    PS2_CS(0);
    psx_buf[0] = ps2_transfer(START_CMD);
    psx_buf[1] = ps2_transfer(ASK_DAT_CMD);
    psx_buf[2] = ps2_transfer(psx_buf[0]);
    psx_buf[3] = ps2_transfer(psx_buf[0]);
    psx_buf[4] = ps2_transfer(psx_buf[0]);
    psx_buf[5] = ps2_transfer(psx_buf[0]);
    psx_buf[6] = ps2_transfer(psx_buf[0]);
    psx_buf[7] = ps2_transfer(psx_buf[0]);
    psx_buf[8] = ps2_transfer(psx_buf[0]);
    PS2_CS(1);

    //printf("%x    %x    %x    %x    %x    %x    %x    %x    %x\r\n",psx_buf[0],psx_buf[1],psx_buf[2],psx_buf[3],psx_buf[4],psx_buf[5],psx_buf[6],psx_buf[7],psx_buf[8]);
	// printf("%d    %d    %d    %d    %d    %d    %d    %d    %d\r\n",psx_buf[0],psx_buf[1],psx_buf[2],psx_buf[3],psx_buf[4],psx_buf[5],psx_buf[6],psx_buf[7],psx_buf[8]);
}
