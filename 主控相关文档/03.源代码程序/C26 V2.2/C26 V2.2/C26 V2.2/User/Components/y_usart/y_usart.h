/****************************************************************************
 *	@笔者	：	Q
 *	@日期	：	2023年2月8日
 *	@所属	：	杭州友辉科技
 *	@功能	：	存放usart串口相关的函数
 ****************************************************************************/
#ifndef _Y_USART_H_
#define _Y_USART_H_
#include "main.h"

#define UART_BUF_SIZE 512 // 定义最大接收字节数

extern char uart1_receive_buf[UART_BUF_SIZE]; // 接收缓冲,最大UART_BUF_SIZE个字节.末字节为换行符
extern uint16_t uart1_get_ok;				   // 接收完成标记
extern char uart1_mode;					   /* 指令的模式 */

extern char uart2_receive_buf[UART_BUF_SIZE];
extern uint16_t uart2_get_ok;
extern char uart2_mode;

extern char uart3_receive_buf[UART_BUF_SIZE];
extern uint16_t uart3_get_ok;
extern char uart3_mode;

extern char uart5_receive_buf[UART_BUF_SIZE];
extern uint16_t uart5_get_ok;
extern char uart5_mode;

/*******串口相关函数声明*******/
void uart1_init(uint32_t BaudRate); /* 初始化串口1 */
void uart1_send_str(char *s);       /* Usart1发送字符串 */

void uart2_init(u32 baud);
void uart2_send_str(char *s);

void uart3_init(u32 baud);
void uart3_send_str(char *s);

void uart5_init(u32 baud);
void uart5_send_str(char *s);

#endif
