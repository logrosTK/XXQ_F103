#include "ultrasonic/ultrasonic.h"


// 延时 用于等待应答时的超时判断 移植时需修改
void ultrasonic_i2c_delay() // 每步的间隔 用于等待电平稳定和控制通讯速率
{
    unsigned char i;
    i = 20;
    while (--i)
    {
    }
}


/*******************************************************************************
 * 函 数 名       : ultrasonic_i2c_start
 * 函数功能		 : 产生I2C起始信号
 * 输    入       : 无
 * 输    出    	 : 无
 *******************************************************************************/
void ultrasonic_i2c_start(void)
{
    I2C_SDA_H();
    I2C_SCL_H();
    ultrasonic_i2c_delay();

    I2C_SDA_L(); // 当SCL为高电平时，SDA由高变为低
    ultrasonic_i2c_delay();
    I2C_SCL_L(); // 钳住I2C总线，准备发送或接收数据
}

/*******************************************************************************
 * 函 数 名         : ultrasonic_i2c_stop
 * 函数功能		   : 产生I2C停止信号
 * 输    入         : 无
 * 输    出         : 无
 *******************************************************************************/
void ultrasonic_i2c_stop(void)
{
    I2C_SCL_L();
    I2C_SDA_L();
    ultrasonic_i2c_delay();
    I2C_SCL_H();
    ultrasonic_i2c_delay();

    I2C_SDA_H(); // 当SCL为高电平时，SDA由低变为高
    ultrasonic_i2c_delay();
}

/*******************************************************************************
 * 函 数 名         : ultrasonic_i2c_ack
 * 函数功能		   : 产生ACK应答
 * 输    入         : 无
 * 输    出         : 无
 *******************************************************************************/
void ultrasonic_i2c_ack(void)
{
    I2C_SDA_L(); // SDA为低电平
    ultrasonic_i2c_delay();

    I2C_SCL_H();
    ultrasonic_i2c_delay();
    I2C_SCL_L();
    I2C_SDA_H();
    ultrasonic_i2c_delay();
}

/*******************************************************************************
 * 函 数 名         : ultrasonic_i2c_nack
 * 函数功能		   : 产生NACK非应答
 * 输    入         : 无
 * 输    出         : 无
 *******************************************************************************/
void ultrasonic_i2c_nack(void)
{
    I2C_SDA_H(); // SDA为高电平
    ultrasonic_i2c_delay();

    I2C_SCL_H();
    ultrasonic_i2c_delay();
    I2C_SCL_L();
    ultrasonic_i2c_delay();
}

/*******************************************************************************
* 函 数 名         : ultrasonic_i2c_wait_ack
* 函数功能		   : 等待应答信号到来
* 输    入         : 无
* 输    出         : 1，接收应答失败
                     0，接收应答成功
*******************************************************************************/
uint8_t ultrasonic_i2c_wait_ack(void)
{
    uint16_t time_temp = 0;

    I2C_SDA_H();
    ultrasonic_i2c_delay();
    I2C_SCL_H();
    ultrasonic_i2c_delay();
    while (I2C_SDA_Read()) // 等待SDA为低电平
    {
        time_temp++;
        ultrasonic_i2c_delay();
        if (time_temp > I2C_TIMEOUT_TIMES) // 超时则强制结束I2C通信
        {
            ultrasonic_i2c_stop();
            return 1;
        }
    }
    I2C_SCL_L();
    ultrasonic_i2c_delay();
    return 0;
}

/*******************************************************************************
 * 函 数 名         : ultrasonic_i2c_write_byte
 * 函数功能		   : I2C发送一个字节
 * 输    入         : dat：发送一个字节
 * 输    出         : 无
 *******************************************************************************/
void ultrasonic_i2c_write_byte(uint8_t dat)
{
    uint8_t i = 0;

    I2C_SCL_L();
    ultrasonic_i2c_delay();
    for (i = 0; i < 8; i++) // 循环8次将一个字节传出，先传高再传低位
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

/*******************************************************************************
 * 函 数 名         : ultrasonic_i2c_read_byte
 * 函数功能		   : I2C读一个字节
 * 输    入         : ack = 1时，发送ACK，ack = 0，发送nACK
 * 输    出         : 应答或非应答
 *******************************************************************************/
uint8_t ultrasonic_i2c_read_byte(uint8_t ack)
{
    uint8_t i = 0, receive = 0;

    for (i = 0; i < 8; i++) // 循环8次将一个字节读出，先读高再传低位
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


/* 超时回调函数 */
static uint8_t I2C_TIMEOUT_UserCallback(uint8_t errorCode)
{
    printf("ultrasonic iic errorCode = %d\r\n", errorCode);
    ultrasonic_i2c_stop();  // 超时后发送停止信号，释放总线
    return 0;
}

/* 幻彩超声波初始化 */
uint8_t ultrasonic_Init(void)
{
    uint8_t ack;
        
    // 验证0x2D地址是否为ultrasonic
    ultrasonic_i2c_start();
    ultrasonic_i2c_write_byte((0x2D << 1) | 0);  // 写操作
    ack = ultrasonic_i2c_wait_ack();
    ultrasonic_i2c_stop();
    
    if (ack == 0)
        return 1;  // 检测到设备
    else
        return 0;  // 未检测到设备
}



// R探头RGB灯控制
uint8_t ultrasonic_rgb_r(uint8_t r,uint8_t g,uint8_t b)
{
	ultrasonic_i2c_start();
	ultrasonic_i2c_write_byte(0x5a);
	if (ultrasonic_i2c_wait_ack()) return I2C_TIMEOUT_UserCallback(0);
	ultrasonic_i2c_write_byte(0x00);
	if (ultrasonic_i2c_wait_ack()) return I2C_TIMEOUT_UserCallback(1);
	ultrasonic_i2c_write_byte(r); // 写寄存器地址
	if (ultrasonic_i2c_wait_ack()) return I2C_TIMEOUT_UserCallback(2);
	ultrasonic_i2c_stop();
	delay_us(1000);
	
	ultrasonic_i2c_start();
	ultrasonic_i2c_write_byte(0x5a);
	if (ultrasonic_i2c_wait_ack()) return I2C_TIMEOUT_UserCallback(3);
	ultrasonic_i2c_write_byte(0x01);
	if (ultrasonic_i2c_wait_ack()) return I2C_TIMEOUT_UserCallback(4);
	ultrasonic_i2c_write_byte(g); // 写寄存器地址
	if (ultrasonic_i2c_wait_ack()) return I2C_TIMEOUT_UserCallback(5);
	ultrasonic_i2c_stop();
	delay_us(1000);
	
	ultrasonic_i2c_start();
	ultrasonic_i2c_write_byte(0x5a);
	if (ultrasonic_i2c_wait_ack()) return I2C_TIMEOUT_UserCallback(6);
	ultrasonic_i2c_write_byte(0x02);
	if (ultrasonic_i2c_wait_ack()) return I2C_TIMEOUT_UserCallback(7);
	ultrasonic_i2c_write_byte(b); // 写寄存器地址
	if (ultrasonic_i2c_wait_ack()) return I2C_TIMEOUT_UserCallback(8);
	ultrasonic_i2c_stop();
	delay_us(1000);

    return 1;
}

// T探头RGB灯控制
uint8_t ultrasonic_rgb_t(uint8_t r,uint8_t g,uint8_t b)
{
	ultrasonic_i2c_start();
	ultrasonic_i2c_write_byte(0x5a);
	if (ultrasonic_i2c_wait_ack()) return I2C_TIMEOUT_UserCallback(9);
	ultrasonic_i2c_write_byte(0x03);
	if (ultrasonic_i2c_wait_ack()) return I2C_TIMEOUT_UserCallback(10);
	ultrasonic_i2c_write_byte(r); // 写寄存器地址
	if (ultrasonic_i2c_wait_ack()) return I2C_TIMEOUT_UserCallback(11);
	ultrasonic_i2c_stop();
	delay_us(1000);
	
	ultrasonic_i2c_start();
	ultrasonic_i2c_write_byte(0x5a);
	if (ultrasonic_i2c_wait_ack()) return I2C_TIMEOUT_UserCallback(12);
	ultrasonic_i2c_write_byte(0x04);
	if (ultrasonic_i2c_wait_ack()) return I2C_TIMEOUT_UserCallback(13);
	ultrasonic_i2c_write_byte(g); // 写寄存器地址
	if (ultrasonic_i2c_wait_ack()) return I2C_TIMEOUT_UserCallback(14);
	ultrasonic_i2c_stop();
	delay_us(1000);
	
	ultrasonic_i2c_start();
	ultrasonic_i2c_write_byte(0x5a);
	if (ultrasonic_i2c_wait_ack()) return I2C_TIMEOUT_UserCallback(15);
	ultrasonic_i2c_write_byte(0x05);
	if (ultrasonic_i2c_wait_ack()) return I2C_TIMEOUT_UserCallback(16);
	ultrasonic_i2c_write_byte(b); // 写寄存器地址
	if (ultrasonic_i2c_wait_ack()) return I2C_TIMEOUT_UserCallback(17);
	ultrasonic_i2c_stop();
	delay_us(1000);

    return 1;
}

/**
 *@beaf 超声波开始测距，需要等待100ms测量时间
 *@param 无
 */
uint8_t ultrasonic_start_measuring(void)
{
	//写地址为0X5a 寄存器地址0x10
	ultrasonic_i2c_start();
	ultrasonic_i2c_write_byte(0x5a);
	if (ultrasonic_i2c_wait_ack()) return I2C_TIMEOUT_UserCallback(18);
	ultrasonic_i2c_write_byte(0x10);
	if (ultrasonic_i2c_wait_ack()) return I2C_TIMEOUT_UserCallback(19);
	ultrasonic_i2c_write_byte(1); //写命令0X01,0X01为开始测量命令 
	if (ultrasonic_i2c_wait_ack()) return I2C_TIMEOUT_UserCallback(20);
	ultrasonic_i2c_stop();

    return 1;
}


/**
 *@beaf 超声波测距 cm
 *@param 无
 *@retval distance 测试的距离
 */
float ultrasonic_read_distance(void)
{
	u8 value_H=0,value_L=0;
	float distance=0;
	
	ultrasonic_i2c_start();  	 	   
	ultrasonic_i2c_write_byte(0X5b);     //地址为0X5b 读2个8位距离数据          		   
	if (ultrasonic_i2c_wait_ack()) return I2C_TIMEOUT_UserCallback(21);	
	value_H=ultrasonic_i2c_read_byte(1);	
	value_L=ultrasonic_i2c_read_byte(0);	
	ultrasonic_i2c_stop();//产生一个停止条件	
	// 340m/s = 0.017cm/us
	distance=(value_H<<8|value_L)* 0.017;
	return distance;
}

