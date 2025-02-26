#ifndef __USART_H
#define __USART_H

#include "sys.h" 
#include "tinyprintf.h" 

#define USART_REC_LEN  			100  	//定义最大接收字节数 200
extern u8  USART_RX_BUF[USART_REC_LEN]; //接收缓冲,最大USART_REC_LEN个字节.末字节为换行符 
extern u16 USART_RX_STA;         		//接收状态标记	
//如果想串口中断接收，请不要注释以下宏定义
void uart_init(u32 bound);
#endif


