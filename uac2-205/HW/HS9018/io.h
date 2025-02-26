#ifndef __LED_H
#define __LED_H
#include "sys.h"
#include "usart.h"  

//st 205	
//usb led
#define led_r1 PDout(10)
#define led_g2 PDout(11)
//play led
#define led_r3 PDout(12)
#define led_g4 PDout(13)

//xtal
#define SEL_480	{PDout(0)=0;PDout(1)=1;}
#define SEL_441	{PDout(1)=0;PDout(0)=1;}
#define SEL_NONE	{PDout(0)=0;PDout(1)=0;}

//9018 resetb
#define DAC_EN	PDout(2)=1;
#define DAC_DIS	PDout(2)=0;

//dcdc ldo ctrl
#define APWR1_EN	PDout(14)=1;
#define APWR1_DIS	PDout(14)=0;
#define APWR2_EN	PDout(15)=1;
#define APWR2_DIS	PDout(15)=0;

//led set
#define BSP_AUDIOPLAY led_r3=0;led_g4=1;
#define BSP_AUDIOSTOP led_r3=0;led_g4=0;
#define BSP_AUDIOWARN led_r3=1;led_g4=1;
#define BSP_AUDIOERR led_r3=1;led_g4=0;

#define BSP_USBREADY  led_r1=0;led_g2=1;
#define BSP_USBSLEEP led_r1=1;led_g2=1;
#define BSP_USBLOST led_r1=1;led_g2=0;
    


void Board_Init(void);//初始化		 				    
#endif
