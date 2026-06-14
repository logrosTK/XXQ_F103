/*
 * ================================================================================
 * @文件名称: ultrasonic.c
 * @功能描述: 幻彩超声波传感器驱动，基于软件I2C通信
 *           支持RGB灯控制、超声波测距等功能
 * @所属模块: Components/ultrasonic
 * @依赖: ultrasonic.h
 * @硬件: I2C地址 - R探头RGB: 0x5A(写), T探头RGB: 0x5A(写)
 *                   超声波测距: 0x5A(写命令), 0x5B(读数据)
 *                   设备检测: 0x2D
 * ================================================================================
 */

#include "ultrasonic/ultrasonic.h"

#define ULTRASONIC_MEASURE_WAIT_MS 100U


/*
 * 函数名称: ultrasonic_i2c_delay
 * 功能描述: 超声波I2C通信每步之间的延时，用于等待电平稳定
 * 参数说明: 无
 * 返回值:   无
 * 使用说明: 内部函数，控制I2C通信速率
 */
void ultrasonic_i2c_delay()
{
    unsigned char i;
    i = 20;
    while (--i)
    {
    }
}


/*
 * 函数名称: ultrasonic_i2c_start
 * 功能描述: 产生超声波I2C总线起始信号
 * 参数说明: 无
 * 返回值:   无
 * 使用说明: 内部函数，每次I2C通信开始时调用
 */
void ultrasonic_i2c_start(void)
{
    I2C_SDA_H();
    I2C_SCL_H();
    ultrasonic_i2c_delay();

    I2C_SDA_L();
    ultrasonic_i2c_delay();
    I2C_SCL_L();
}

/*
 * 函数名称: ultrasonic_i2c_stop
 * 功能描述: 产生超声波I2C总线停止信号
 * 参数说明: 无
 * 返回值:   无
 * 使用说明: 内部函数，每次I2C通信结束时调用
 */
void ultrasonic_i2c_stop(void)
{
    I2C_SCL_L();
    I2C_SDA_L();
    ultrasonic_i2c_delay();
    I2C_SCL_H();
    ultrasonic_i2c_delay();

    I2C_SDA_H();
    ultrasonic_i2c_delay();
}

/*
 * 函数名称: ultrasonic_i2c_ack
 * 功能描述: 产生ACK应答信号（拉低SDA）
 * 参数说明: 无
 * 返回值:   无
 */
void ultrasonic_i2c_ack(void)
{
    I2C_SDA_L();
    ultrasonic_i2c_delay();

    I2C_SCL_H();
    ultrasonic_i2c_delay();
    I2C_SCL_L();
    I2C_SDA_H();
    ultrasonic_i2c_delay();
}

/*
 * 函数名称: ultrasonic_i2c_nack
 * 功能描述: 产生NACK非应答信号（保持SDA高电平）
 * 参数说明: 无
 * 返回值:   无
 */
void ultrasonic_i2c_nack(void)
{
    I2C_SDA_H();
    ultrasonic_i2c_delay();

    I2C_SCL_H();
    ultrasonic_i2c_delay();
    I2C_SCL_L();
    ultrasonic_i2c_delay();
}

/*
 * 函数名称: ultrasonic_i2c_wait_ack
 * 功能描述: 等待从机应答信号，超时则强制结束通信
 * 参数说明: 无
 * 返回值:   0 - 应答成功
 *           1 - 应答超时失败
 * 使用说明: 发送地址或数据后调用，等待SDA被从机拉低
 */
uint8_t ultrasonic_i2c_wait_ack(void)
{
    uint16_t time_temp = 0;

    I2C_SDA_H();
    ultrasonic_i2c_delay();
    I2C_SCL_H();
    ultrasonic_i2c_delay();
    while (I2C_SDA_Read())
    {
        time_temp++;
        ultrasonic_i2c_delay();
        if (time_temp > I2C_TIMEOUT_TIMES)
        {
            ultrasonic_i2c_stop();
            return 1;
        }
    }
    I2C_SCL_L();
    ultrasonic_i2c_delay();
    return 0;
}

/*
 * 函数名称: ultrasonic_i2c_write_byte
 * 功能描述: I2C发送一个字节（MSB先发）
 * 参数说明: dat - 要发送的字节
 * 返回值:   无
 */
void ultrasonic_i2c_write_byte(uint8_t dat)
{
    uint8_t i = 0;

    I2C_SCL_L();
    ultrasonic_i2c_delay();
    for (i = 0; i < 8; i++)
    {
        if ((dat & 0x80) > 0)
            I2C_SDA_H();
        else
            I2C_SDA_L();
        dat <<= 1;
        I2C_SCL_H();
        ultrasonic_i2c_delay();
        I2C_SCL_L();
        ultrasonic_i2c_delay();
    }
}

/*
 * 函数名称: ultrasonic_i2c_read_byte
 * 功能描述: I2C接收一个字节（MSB先收）
 * 参数说明: ack - 1: 发送ACK继续接收, 0: 发送NACK停止接收
 * 返回值:   接收到的字节
 */
uint8_t ultrasonic_i2c_read_byte(uint8_t ack)
{
    uint8_t i = 0, receive = 0;

    for (i = 0; i < 8; i++)
    {
        I2C_SCL_H();
        receive <<= 1;
        if (I2C_SDA_Read())
            receive++;
        ultrasonic_i2c_delay();
        I2C_SCL_L();
        ultrasonic_i2c_delay();
    }
    if (!ack)
        ultrasonic_i2c_nack();
    else
        ultrasonic_i2c_ack();

    return receive;
}


/*
 * 函数名称: I2C_TIMEOUT_UserCallback
 * 功能描述: I2C超时回调函数，打印错误码并发送停止信号释放总线
 * 参数说明: errorCode - 错误码（标识哪个步骤超时）
 * 返回值:   0
 * 使用说明: 内部函数，I2C通信超时时自动调用
 */
static uint8_t I2C_TIMEOUT_UserCallback(uint8_t errorCode)
{
    printf("ultrasonic iic errorCode = %d\r\n", errorCode);
    ultrasonic_i2c_stop();
    return 0;
}

/*
 * 函数名称: ultrasonic_Init
 * 功能描述: 初始化幻彩超声波传感器，检测设备是否存在
 *           通过向0x2D地址发送写命令检测设备应答
 * 参数说明: 无
 * 返回值:   1 - 检测到设备，初始化成功
 *           0 - 未检测到设备，初始化失败
 * 使用说明: 在传感器初始化阶段调用一次
 *           返回值用于判断超声波传感器是否正确连接
 */
uint8_t ultrasonic_Init(void)
{
    uint8_t ack;
        
    ultrasonic_i2c_start();
    ultrasonic_i2c_write_byte((0x2D << 1) | 0);
    ack = ultrasonic_i2c_wait_ack();
    ultrasonic_i2c_stop();
    
    if (ack == 0)
        return 1;
    else
        return 0;
}



/*
 * 函数名称: ultrasonic_rgb_r
 * 功能描述: 控制R探头（接收探头）的RGB灯颜色
 *           分别写入R/G/B三个寄存器的值
 * 参数说明: r - 红色分量 (0~255)
 *          g - 绿色分量 (0~255)
 *          b - 蓝色分量 (0~255)
 * 返回值:   1 - 设置成功
 *           0 - I2C通信超时（通过回调函数处理）
 * 使用说明: 每次设置需连续3次I2C通信写入R/G/B
 *           示例: ultrasonic_rgb_r(255, 0, 0);  // 设置为红色
 */
uint8_t ultrasonic_rgb_r(uint8_t r,uint8_t g,uint8_t b)
{
	ultrasonic_i2c_start();
	ultrasonic_i2c_write_byte(0x5a);
	if (ultrasonic_i2c_wait_ack()) return I2C_TIMEOUT_UserCallback(0);
	ultrasonic_i2c_write_byte(0x00);
	if (ultrasonic_i2c_wait_ack()) return I2C_TIMEOUT_UserCallback(1);
	ultrasonic_i2c_write_byte(r);
	if (ultrasonic_i2c_wait_ack()) return I2C_TIMEOUT_UserCallback(2);
	ultrasonic_i2c_stop();
	delay_us(1000);
	
	ultrasonic_i2c_start();
	ultrasonic_i2c_write_byte(0x5a);
	if (ultrasonic_i2c_wait_ack()) return I2C_TIMEOUT_UserCallback(3);
	ultrasonic_i2c_write_byte(0x01);
	if (ultrasonic_i2c_wait_ack()) return I2C_TIMEOUT_UserCallback(4);
	ultrasonic_i2c_write_byte(g);
	if (ultrasonic_i2c_wait_ack()) return I2C_TIMEOUT_UserCallback(5);
	ultrasonic_i2c_stop();
	delay_us(1000);
	
	ultrasonic_i2c_start();
	ultrasonic_i2c_write_byte(0x5a);
	if (ultrasonic_i2c_wait_ack()) return I2C_TIMEOUT_UserCallback(6);
	ultrasonic_i2c_write_byte(0x02);
	if (ultrasonic_i2c_wait_ack()) return I2C_TIMEOUT_UserCallback(7);
	ultrasonic_i2c_write_byte(b);
	if (ultrasonic_i2c_wait_ack()) return I2C_TIMEOUT_UserCallback(8);
	ultrasonic_i2c_stop();
	delay_us(1000);

    return 1;
}

/*
 * 函数名称: ultrasonic_rgb_t
 * 功能描述: 控制T探头（发射探头）的RGB灯颜色
 *           分别写入R/G/B三个寄存器的值（寄存器地址0x03/0x04/0x05）
 * 参数说明: r - 红色分量 (0~255)
 *          g - 绿色分量 (0~255)
 *          b - 蓝色分量 (0~255)
 * 返回值:   1 - 设置成功
 *           0 - I2C通信超时
 * 使用说明: 同ultrasonic_rgb_r，操作T探头
 *           示例: ultrasonic_rgb_t(0, 255, 0);  // 设置为绿色
 */
uint8_t ultrasonic_rgb_t(uint8_t r,uint8_t g,uint8_t b)
{
	ultrasonic_i2c_start();
	ultrasonic_i2c_write_byte(0x5a);
	if (ultrasonic_i2c_wait_ack()) return I2C_TIMEOUT_UserCallback(9);
	ultrasonic_i2c_write_byte(0x03);
	if (ultrasonic_i2c_wait_ack()) return I2C_TIMEOUT_UserCallback(10);
	ultrasonic_i2c_write_byte(r);
	if (ultrasonic_i2c_wait_ack()) return I2C_TIMEOUT_UserCallback(11);
	ultrasonic_i2c_stop();
	delay_us(1000);
	
	ultrasonic_i2c_start();
	ultrasonic_i2c_write_byte(0x5a);
	if (ultrasonic_i2c_wait_ack()) return I2C_TIMEOUT_UserCallback(12);
	ultrasonic_i2c_write_byte(0x04);
	if (ultrasonic_i2c_wait_ack()) return I2C_TIMEOUT_UserCallback(13);
	ultrasonic_i2c_write_byte(g);
	if (ultrasonic_i2c_wait_ack()) return I2C_TIMEOUT_UserCallback(14);
	ultrasonic_i2c_stop();
	delay_us(1000);
	
	ultrasonic_i2c_start();
	ultrasonic_i2c_write_byte(0x5a);
	if (ultrasonic_i2c_wait_ack()) return I2C_TIMEOUT_UserCallback(15);
	ultrasonic_i2c_write_byte(0x05);
	if (ultrasonic_i2c_wait_ack()) return I2C_TIMEOUT_UserCallback(16);
	ultrasonic_i2c_write_byte(b);
	if (ultrasonic_i2c_wait_ack()) return I2C_TIMEOUT_UserCallback(17);
	ultrasonic_i2c_stop();
	delay_us(1000);

    return 1;
}

/*
 * 函数名称: ultrasonic_start_measuring
 * 功能描述: 发送超声波测距开始命令
 *           向寄存器0x10写入0x01启动测量
 * 参数说明: 无
 * 返回值:   1 - 命令发送成功
 *           0 - I2C通信超时
 * 使用说明: 调用后需等待约100ms测量时间
 *           然后调用ultrasonic_read_distance()读取距离值
 *           示例: ultrasonic_start_measuring();
 *                delay_ms(100);
 *                float dist = ultrasonic_read_distance();
 */
uint8_t ultrasonic_start_measuring(void)
{
	ultrasonic_i2c_start();
	ultrasonic_i2c_write_byte(0x5a);
	if (ultrasonic_i2c_wait_ack()) return I2C_TIMEOUT_UserCallback(18);
	ultrasonic_i2c_write_byte(0x10);
	if (ultrasonic_i2c_wait_ack()) return I2C_TIMEOUT_UserCallback(19);
	ultrasonic_i2c_write_byte(1);
	if (ultrasonic_i2c_wait_ack()) return I2C_TIMEOUT_UserCallback(20);
	ultrasonic_i2c_stop();

    return 1;
}


/*
 * 函数名称: ultrasonic_read_distance
 * 功能描述: 读取超声波测距结果
 *           从0x5B地址读取2字节数据，换算为厘米距离
 * 参数说明: 无
 * 返回值:   float - 测量距离，单位cm
 *           计算公式: distance = (value_H<<8 | value_L) * 0.017
 *           声速340m/s = 0.017cm/us（往返时间换算）
 * 使用说明: 需在ultrasonic_start_measuring()之后等待100ms再调用
 *           示例: float dist_cm = ultrasonic_read_distance();
 */
float ultrasonic_read_distance(void)
{
	u8 value_H=0,value_L=0;
	float distance=0;
	
	ultrasonic_i2c_start();  	 	   
	ultrasonic_i2c_write_byte(0X5b);
	if (ultrasonic_i2c_wait_ack()) return I2C_TIMEOUT_UserCallback(21);	
	value_H=ultrasonic_i2c_read_byte(1);	
	value_L=ultrasonic_i2c_read_byte(0);	
	ultrasonic_i2c_stop();
	distance=(value_H<<8|value_L)* 0.017;
	return distance;
}

/*
 * 函数名称: ultrasonic_measure_once
 * 功能描述: 执行一次完整超声波测距
 * 参数说明: distance_cm - 输出的距离值指针，单位cm
 * 返回值:   1 - 测距成功
 *           0 - 参数无效或I2C通信失败
 */
uint8_t ultrasonic_measure_once(float *distance_cm)
{
    if (distance_cm == NULL)
    {
        return 0;
    }

    if (!ultrasonic_start_measuring())
    {
        return 0;
    }

    delay_ms(ULTRASONIC_MEASURE_WAIT_MS);
    *distance_cm = ultrasonic_read_distance();

    return 1;
}

/*
 * 函数名称: ultrasonic_self_test
 * 功能描述: 执行设备检测、点亮探头RGB并读取一次距离
 * 参数说明: distance_cm - 输出的距离值指针，单位cm
 * 返回值:   1 - 自检成功
 *           0 - 自检失败
 */
uint8_t ultrasonic_self_test(float *distance_cm)
{
    if (!ultrasonic_Init())
    {
        return 0;
    }

    if (!ultrasonic_rgb_r(0, 255, 0))
    {
        return 0;
    }

    if (!ultrasonic_rgb_t(0, 255, 0))
    {
        return 0;
    }

    return ultrasonic_measure_once(distance_cm);
}

