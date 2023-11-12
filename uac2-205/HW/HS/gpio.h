#ifndef __LED_H
#define __LED_H
#include "sys.h"

//st 205	
//r
#define LED1 PAout(0)
//g
#define LED2 PAout(1)
//b
#define LED3 PAout(2)
//rsv
#define LED4 PAout(4)

#define LEDON	{LED1=0;LED2=1;LED3=0;}
#define LEDOFF	{LED1=0;LED2=0;LED3=0;}
#define LED_GRN	{LED1=0;LED2=1;LED3=0;}
#define LED_YEL	{LED1=1;LED2=1;LED3=0;}
#define LED_RED	{LED1=1;LED2=0;LED3=0;}

#define LED_BLU	{LED3=1;LED1=0;LED2=0;}
#define LED_CYA	{LED3=1;LED2=1;LED1=0;}
#define LED_VLT	{LED3=1;LED2=0;LED1=1;}
#define LED_WHI	{LED3=1;LED2=1;LED1=1;}

#define SEL_24	{PBout(8)=0;PBout(9)=1;}
#define SEL_22	{PBout(9)=0;PBout(8)=1;}
#define SEL_NONE	{PBout(8)=0;PBout(9)=0;}

#define DAC_EN	PCout(10)=1;
#define DAC_DIS	PCout(10)=0;

#define APWR_STAGE1_EN	PBout(8)=1;
#define APWR_STAGE1_DIS	PBout(8)=0;
#define APWR_EN		PBout(9)=1;
#define APWR_DIS	PBout(9)=0;
    


void Board_Init(void);//初始化		 				    
#endif
