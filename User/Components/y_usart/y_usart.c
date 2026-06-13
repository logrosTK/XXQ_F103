#include "y_usart/y_usart.h"

char uart1_receive_buf[UART_BUF_SIZE]; // 接收缓冲,最大UART_BUF_SIZE个字节.末字节为换行符
uint16_t uart1_get_ok;				   // 接收完成标记
char uart1_mode;					   /* 指令的模式 */

char uart2_receive_buf[UART_BUF_SIZE];
uint16_t uart2_get_ok;
char uart2_mode;

char uart3_receive_buf[UART_BUF_SIZE];
uint16_t uart3_get_ok;
char uart3_mode;

char uart5_receive_buf[UART_BUF_SIZE];
uint16_t uart5_get_ok;
char uart5_mode;

/* 初始化串口1 */
void uart1_init(uint32_t BaudRate)
{
	USART_InitTypeDef USART_InitStructure;
	GPIO_InitTypeDef GPIO_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	/* 使能端口时钟 */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_USART1, ENABLE);

	USART_DeInit(USART1);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;		/* PA.9 */
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP; /* 复用推挽输出 */
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	USART_InitStructure.USART_BaudRate = BaudRate;									/* 串口波特率 */
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;						/* 字长为8位数据格式 */
	USART_InitStructure.USART_StopBits = USART_StopBits_1;							/* 字长为8位数据格式 */
	USART_InitStructure.USART_Parity = USART_Parity_No;								/* 无奇偶校验位 */
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;					/* 收发模式 */
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None; /* 无硬件数据流控制 */
	USART_Init(USART1, &USART_InitStructure);

	NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1; /* 抢占优先级 */
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;		  /* 子优先级 */
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			  /* IRQ通道使能 */
	NVIC_Init(&NVIC_InitStructure);

	USART_ITConfig(USART1, USART_IT_RXNE, ENABLE); /* 开启串口接受中断 */
	USART_ITConfig(USART1, USART_IT_TXE, DISABLE); /* 禁止串口发送中断 */

	USART_Cmd(USART1, ENABLE); /* 使能串口1  */
}

/***********************************************
	函数名称:	uart3_init()
	功能介绍:	初始化串口3
	函数参数:	baud 波特率
	返回值:		无
 ***********************************************/
void uart2_init(u32 baud)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	USART_InitStructure.USART_BaudRate = baud;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	USART_Init(USART2, &USART_InitStructure);

	NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);
	USART_ITConfig(USART2, USART_IT_TXE, DISABLE); /* 禁止串口发送中断 */

	USART_Cmd(USART2, ENABLE);
}

/***********************************************
	函数名称:	uart3_init()
	功能介绍:	初始化串口3
	函数参数:	baud 波特率
	返回值:		无
 ***********************************************/
void uart3_init(u32 baud)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_OD;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	USART_InitStructure.USART_BaudRate = baud;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	USART_Init(USART3, &USART_InitStructure);

	NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	USART_ITConfig(USART3, USART_IT_RXNE, ENABLE);
	USART_ITConfig(USART3, USART_IT_TXE, DISABLE); /* 禁止串口发送中断 */

	USART_Cmd(USART3, ENABLE);
}

/***********************************************
	函数名称:	uart5_init()
	功能介绍:	初始化串口5
	函数参数:	baud 波特率
	返回值:		无
 ***********************************************/
void uart5_init(u32 baud)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_UART5, ENABLE);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_OD;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	USART_InitStructure.USART_BaudRate = baud;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	USART_Init(UART5, &USART_InitStructure);
	USART_HalfDuplexCmd(UART5, ENABLE); // 注意这个，启动半双工模式

	NVIC_InitStructure.NVIC_IRQChannel = UART5_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	USART_ITConfig(UART5, USART_IT_RXNE, ENABLE);
	USART_ITConfig(UART5, USART_IT_TXE, DISABLE); /* 禁止串口发送中断 */

	USART_Cmd(UART5, ENABLE);
}

int __io_putchar(int ch)
{
	uint32_t timeout = 100000U;

	while ((USART1->SR & USART_SR_TXE) == 0U)
	{
		if (timeout == 0U)
		{
			return ch;
		}
		timeout--;
	}

	USART1->DR = (u8)ch;
	return ch;
}

/* 重定义fputc函数,兼容不同C库的printf输出路径 */
int fputc(int ch, FILE *f)
{
	(void)f;
	return __io_putchar(ch);
}

/***********************************************
	功能介绍：	串口1发送字符串
	函数参数：	*s 发送的字符串
	返回值：		无
 ***********************************************/
void uart1_send_str(char *s)
{
	while (*s)
	{
		USART1->DR = (uint8_t)*s++;
		while (!(USART1->SR & USART_SR_TXE))
			;
	}
}

/***********************************************
	函数名称:	uart3_send_str()
	功能介绍:	串口3发送字符串
	函数参数:	*s 发送的字符串
	返回值:		无
 ***********************************************/
void uart2_send_str(char *s)
{
	while (*s)
	{
		USART2->DR = (uint8_t)*s++;
		while (!(USART2->SR & USART_SR_TXE))
			;
	}
}

/***********************************************
	函数名称:	uart3_send_str()
	功能介绍:	串口3发送字符串
	函数参数:	*s 发送的字符串
	返回值:		无
 ***********************************************/
void uart3_send_str(char *s)
{
	while (*s)
	{
		USART3->DR = (uint8_t)*s++;
		while (!(USART3->SR & USART_SR_TXE))
			;
	}
}

/***********************************************
	函数名称:	uart5_send_str()
	功能介绍:	串口5发送字符串
	函数参数:	*s 发送的字符串
	返回值:		无
 ***********************************************/
void uart5_send_str(char *s)
{
	UART5->CR1 &= ~(USART_CR1_RE | USART_CR1_RXNEIE); // 清除接收使能(RE)和接收中断使能(RXNEIE)
	while (*s)
	{
		UART5->DR = (uint8_t)*s++;
		while (!(UART5->SR & USART_SR_TXE))
			;
	}
	UART5->CR1 |= (USART_CR1_RE | USART_CR1_RXNEIE); // 恢复接收使能(RE)和接收中断使能(RXNEIE)
}

/* 串口1中断服务程序 */
void USART1_IRQHandler(void)
{
	static u16 buf_index = 0;

	// 先判断标志位
	if (USART_GetITStatus(USART1, USART_IT_RXNE) == SET)
	{
		// 接收数据
		uint8_t rx_data = USART_ReceiveData(USART1);

		if (uart1_get_ok)
			return;

		if (uart1_mode == 0)
		{
			if (rx_data == '$')
			{
				// 命令模式 $XXX!
				uart1_mode = 1;
			}
			else if (rx_data == '#')
			{
				// 单舵机模式	#000P1500T1000! 类似这种命令
				uart1_mode = 2;
			}
			else if (rx_data == '{')
			{
				// 多舵机模式	{#000P1500T1000!#001P1500T1000!} 多个单舵机命令用大括号括起来
				uart1_mode = 3;
			}
			else if (rx_data == '<')
			{
				// 保存动作组模式	<G0000#000P1500T1000!#001P1500T1000!B000!> 用尖括号括起来 带有组序号
				uart1_mode = 4;
			}
			buf_index = 0;
		}

		uart1_receive_buf[buf_index++] = rx_data;

		if ((uart1_mode == 1) && (rx_data == '!'))
		{
			uart1_receive_buf[buf_index] = '\0';
			uart1_get_ok = 1;
		}
		else if ((uart1_mode == 2) && (rx_data == '!'))
		{
			uart1_receive_buf[buf_index] = '\0';
			uart1_get_ok = 1;
		}
		else if ((uart1_mode == 3) && (rx_data == '}'))
		{
			uart1_receive_buf[buf_index] = '\0';
			uart1_get_ok = 1;
		}
		else if ((uart1_mode == 4) && (rx_data == '>'))
		{
			uart1_receive_buf[buf_index] = '\0';
			uart1_get_ok = 1;
		}

		if (buf_index >= UART_BUF_SIZE)
			buf_index = 0;

		// 手动清除标志位
		USART_ClearITPendingBit(USART1, USART_IT_RXNE);
	}
}

/***********************************************
	函数名称:	void USART2_IRQHandler(void)
	功能介绍:	串口3中断函数
	函数参数:	无
	返回值:		无
 ***********************************************/
void USART2_IRQHandler(void)
{
	static u16 buf_index = 0;

	// 先判断标志位
	if (USART_GetITStatus(USART2, USART_IT_RXNE) == SET)
	{
		// 接收数据
		uint8_t rx_data = USART_ReceiveData(USART2);

		if (uart2_get_ok)
			return;

		if (uart2_mode == 0)
		{
			if (rx_data == '$')
			{
				// 命令模式 $XXX!
				uart2_mode = 1;
			}
			else if (rx_data == '#')
			{
				// 单舵机模式	#000P1500T1000! 类似这种命令
				uart2_mode = 2;
			}
			else if (rx_data == '{')
			{
				// 多舵机模式	{#000P1500T1000!#001P1500T1000!} 多个单舵机命令用大括号括起来
				uart2_mode = 3;
			}
			else if (rx_data == '<')
			{
				// 保存动作组模式	<G0000#000P1500T1000!#001P1500T1000!B000!> 用尖括号括起来 带有组序号
				uart2_mode = 4;
			}
			buf_index = 0;
		}

		uart2_receive_buf[buf_index++] = rx_data;

		if ((uart2_mode == 1) && (rx_data == '!'))
		{
			uart2_receive_buf[buf_index] = '\0';
			uart2_get_ok = 1;
		}
		else if ((uart2_mode == 2) && (rx_data == '!'))
		{
			uart2_receive_buf[buf_index] = '\0';
			uart2_get_ok = 1;
		}
		else if ((uart2_mode == 3) && (rx_data == '}'))
		{
			uart2_receive_buf[buf_index] = '\0';
			uart2_get_ok = 1;
		}
		else if ((uart2_mode == 4) && (rx_data == '>'))
		{
			uart2_receive_buf[buf_index] = '\0';
			uart2_get_ok = 1;
		}

		if (buf_index >= UART_BUF_SIZE)
			buf_index = 0;

		// 手动清除标志位
		USART_ClearITPendingBit(USART2, USART_IT_RXNE);
	}
}

/***********************************************
	函数名称:	void USART3_IRQHandler(void)
	功能介绍:	串口3中断函数
	函数参数:	无
	返回值:		无
 ***********************************************/
void USART3_IRQHandler(void)
{
	static u16 buf_index = 0;

	// 先判断标志位
	if (USART_GetITStatus(USART3, USART_IT_RXNE) == SET)
	{
		// 接收数据
		uint8_t rx_data = USART_ReceiveData(USART3);

		if (uart3_get_ok)
			return;

		if (uart3_mode == 0)
		{
			if (rx_data == '$')
			{
				// 命令模式 $XXX!
				uart3_mode = 1;
			}
			else if (rx_data == '#')
			{
				// 单舵机模式	#000P1500T1000! 类似这种命令
				uart3_mode = 2;
			}
			else if (rx_data == '{')
			{
				// 多舵机模式	{#000P1500T1000!#001P1500T1000!} 多个单舵机命令用大括号括起来
				uart3_mode = 3;
			}
			else if (rx_data == '<')
			{
				// 保存动作组模式	<G0000#000P1500T1000!#001P1500T1000!B000!> 用尖括号括起来 带有组序号
				uart3_mode = 4;
			}
			buf_index = 0;
		}

		uart3_receive_buf[buf_index++] = rx_data;

		if ((uart3_mode == 1) && (rx_data == '!'))
		{
			uart3_receive_buf[buf_index] = '\0';
			uart3_get_ok = 1;
		}
		else if ((uart3_mode == 2) && (rx_data == '!'))
		{
			uart3_receive_buf[buf_index] = '\0';
			uart3_get_ok = 1;
		}
		else if ((uart3_mode == 3) && (rx_data == '}'))
		{
			uart3_receive_buf[buf_index] = '\0';
			uart3_get_ok = 1;
		}
		else if ((uart3_mode == 4) && (rx_data == '>'))
		{
			uart3_receive_buf[buf_index] = '\0';
			uart3_get_ok = 1;
		}

		if (buf_index >= UART_BUF_SIZE)
			buf_index = 0;

		// 手动清除标志位
		USART_ClearITPendingBit(USART3, USART_IT_RXNE);
	}
}

/***********************************************
	函数名称:	void USART3_IRQHandler(void)
	功能介绍:	串口5中断函数
	函数参数:	无
	返回值:		无
 ***********************************************/
void UART5_IRQHandler(void)
{
	static u16 buf_index = 0;

	// 先判断标志位
	if (USART_GetITStatus(UART5, USART_IT_RXNE) == SET)
	{
		// 接收数据
		uint8_t rx_data = USART_ReceiveData(UART5);

		if (uart5_get_ok)
			return;

		if (uart5_mode == 0)
		{
			if (rx_data == '$')
			{
				// 命令模式 $XXX!
				uart5_mode = 1;
			}
			else if (rx_data == '#')
			{
				// 单舵机模式	#000P1500T1000! 类似这种命令
				uart5_mode = 2;
			}
			else if (rx_data == '{')
			{
				// 多舵机模式	{#000P1500T1000!#001P1500T1000!} 多个单舵机命令用大括号括起来
				uart5_mode = 3;
			}
			else if (rx_data == '<')
			{
				// 保存动作组模式	<G0000#000P1500T1000!#001P1500T1000!B000!> 用尖括号括起来 带有组序号
				uart5_mode = 4;
			}
			buf_index = 0;
		}

		uart5_receive_buf[buf_index++] = rx_data;

		if ((uart5_mode == 1) && (rx_data == '!'))
		{
			uart5_receive_buf[buf_index] = '\0';
			uart5_get_ok = 1;
		}
		else if ((uart5_mode == 2) && (rx_data == '!'))
		{
			uart5_receive_buf[buf_index] = '\0';
			uart5_get_ok = 1;
		}
		else if ((uart5_mode == 3) && (rx_data == '}'))
		{
			uart5_receive_buf[buf_index] = '\0';
			uart5_get_ok = 1;
		}
		else if ((uart5_mode == 4) && (rx_data == '>'))
		{
			uart5_receive_buf[buf_index] = '\0';
			uart5_get_ok = 1;
		}

		if (buf_index >= UART_BUF_SIZE)
			buf_index = 0;

		// 手动清除标志位
		USART_ClearITPendingBit(UART5, USART_IT_RXNE);
	}
}
