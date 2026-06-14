/*
 * ================================================================================
 * @文件名称: y_soft_i2c.c
 * @功能描述: 软件模拟I2C总线驱动，用于OLED等I2C设备的通信
 *           实现了I2C的起始/停止/应答/读写等基本时序
 * @所属模块: Components/soft_i2c
 * @依赖: soft_i2c.h, y_delay.h
 * @注意事项: 使用GPIO开漏输出模拟I2C时序，通信速率由i2c_delay控制
 * ================================================================================
 */

#include "soft_i2c/y_soft_i2c.h"
#include "y_delay/y_delay.h"


/*
 * 函数名称: soft_i2c_gpio_init
 * 功能描述: 初始化软件I2C的SCL和SDA引脚，配置为开漏输出模式
 * 参数说明: 无
 * 返回值:   无
 * 使用说明: 在使用I2C通信前调用一次
 *           开漏输出模式支持多设备共享总线和电平仲裁
 */
void soft_i2c_gpio_init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_APB2PeriphClockCmd(SCL_GPIO_CLK | SDA_GPIO_CLK, ENABLE); /* 使能端口时钟 */

    GPIO_InitStructure.GPIO_Pin = SCL_Pin; // 外接I2C
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz; // 速度
    GPIO_Init(SCL_GPIO_Port, &GPIO_InitStructure);    // 对选中管脚初始化

    GPIO_InitStructure.GPIO_Pin = SDA_Pin;         // 外接I2C
    GPIO_Init(SDA_GPIO_Port, &GPIO_InitStructure); // 对选中管脚初始化
}

/*
 * 函数名称: i2c_delay
 * 功能描述: I2C通信每步之间的延时，用于控制通信速率和等待电平稳定
 * 参数说明: 无
 * 返回值:   无
 * 使用说明: 可以根据需要调整延时以改变I2C通信速度
 *           当前为最小延时（空函数），通信速率由GPIO翻转速度决定
 */
void i2c_delay()
{
    // unsigned char i;
    // i = 10;
    // while (--i)
    // {
    // }
}


/*
 * 函数名称: i2c_start
 * 功能描述: 产生I2C总线起始信号（SCL高电平期间SDA由高变低）
 * 参数说明: 无
 * 返回值:   无
 * 使用说明: 每次I2C通信开始时调用
 *           时序: SDA=1, SCL=1 -> SDA=0 -> SCL=0
 */
void i2c_start(void)
{
    I2C_SDA_H();
    I2C_SCL_H();
    i2c_delay();

    I2C_SDA_L(); // 当SCL为高电平时，SDA由高变为低
    i2c_delay();
    I2C_SCL_L(); // 钳住I2C总线，准备发送或接收数据
}

/*
 * 函数名称: i2c_stop
 * 功能描述: 产生I2C总线停止信号（SCL高电平期间SDA由低变高）
 * 参数说明: 无
 * 返回值:   无
 * 使用说明: 每次I2C通信结束时调用
 *           时序: SDA=0, SCL=0 -> SCL=1 -> SDA=1
 */
void i2c_stop(void)
{
    I2C_SCL_L();
    I2C_SDA_L();
    i2c_delay();
    I2C_SCL_H();
    i2c_delay();

    I2C_SDA_H(); // 当SCL为高电平时，SDA由低变为高
    i2c_delay();
}

/*
 * 函数名称: i2c_ack
 * 功能描述: 主机产生ACK应答信号（在第9个时钟周期拉低SDA）
 * 参数说明: 无
 * 返回值:   无
 * 使用说明: 主机接收数据后发送ACK表示继续接收
 */
void i2c_ack(void)
{
    I2C_SDA_L(); // SDA为低电平
    i2c_delay();

    I2C_SCL_H();
    i2c_delay();
    I2C_SCL_L();
    I2C_SDA_H();
    i2c_delay();
}

/*
 * 函数名称: i2c_nack
 * 功能描述: 主机产生NACK非应答信号（在第9个时钟周期保持SDA高电平）
 * 参数说明: 无
 * 返回值:   无
 * 使用说明: 主机接收最后一个字节后发送NACK表示停止接收
 */
void i2c_nack(void)
{
    I2C_SDA_H(); // SDA为高电平
    i2c_delay();

    I2C_SCL_H();
    i2c_delay();
    I2C_SCL_L();
    i2c_delay();
}

/*
 * 函数名称: i2c_wait_ack
 * 功能描述: 主机等待从机应答信号，超时则强制结束通信
 * 参数说明: 无
 * 返回值:   0 - 接收到应答（SDA被从机拉低）
 *           1 - 应答超时（从机未响应）
 * 使用说明: 主机发送地址或数据后调用
 *           超时时间由I2C_TIMEOUT_TIMES定义
 */
uint8_t i2c_wait_ack(void)
{
    uint16_t time_temp = 0;

    I2C_SDA_H();
    i2c_delay();
    I2C_SCL_H();
    i2c_delay();
    while (I2C_SDA_Read()) // 等待SDA为低电平
    {
        time_temp++;
        i2c_delay();
        if (time_temp > I2C_TIMEOUT_TIMES) // 超时则强制结束I2C通信
        {
            i2c_stop();
            return 1;
        }
    }
    I2C_SCL_L();
    i2c_delay();
    return 0;
}

/*
 * 函数名称: i2c_write_byte
 * 功能描述: I2C发送一个字节数据（MSB先发）
 * 参数说明: dat - 要发送的字节数据
 * 返回值:   无
 * 使用说明: 在SCL低电平时设置SDA，SCL上升沿锁存数据
 *           发送8位数据后需要调用i2c_wait_ack等待从机应答
 */
void i2c_write_byte(uint8_t dat)
{
    uint8_t i = 0;

    I2C_SCL_L();
    i2c_delay();
    for (i = 0; i < 8; i++) // 循环8次将一个字节传出，先传高再传低位
    {
        if ((dat & 0x80) > 0)
            I2C_SDA_H();
        else
            I2C_SDA_L();
        dat <<= 1;
        I2C_SCL_H();
        i2c_delay();
        I2C_SCL_L();
        i2c_delay();
    }
}

/*
 * 函数名称: i2c_read_byte
 * 功能描述: I2C接收一个字节数据（MSB先收）
 * 参数说明: ack - 接收后是否发送应答
 *          ack=1: 发送ACK（继续接收下一字节）
 *          ack=0: 发送NACK（停止接收）
 * 返回值:   接收到的字节数据
 * 使用说明: 在SCL上升沿读取SDA数据
 *           连续读取多字节时，除最后一个字节外都应设ack=1
 */
uint8_t i2c_read_byte(uint8_t ack)
{
    uint8_t i = 0, receive = 0;

    for (i = 0; i < 8; i++) // 循环8次将一个字节读出，先读高再传低位
    {
        I2C_SCL_H();
        receive <<= 1;
        if (I2C_SDA_Read())
            receive++;
        i2c_delay();
        I2C_SCL_L();
        i2c_delay();
    }
    if (!ack)
        i2c_nack();
    else
        i2c_ack();

    return receive;
}


