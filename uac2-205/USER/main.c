#include "usart.h"   
#include "gpio.h"  
#include "usbd_audio2_core.h"
#include "audio_out.h"
#include "usbd_usr.h"
#include "usb_conf.h" 	
#include "cm_backtrace.h"

USB_OTG_CORE_HANDLE USB_OTG_dev;
extern vu8 bDeviceState;		//USB连接 情况
u32 cnt=0;

void SysTick_Handler(void)
{
	cnt++;
}

int main(void)
{        
	u8 Divece_STA=0XFF;
	u32 los_cnt=0;
	u32 loscount=0;
	
	cm_backtrace_init("1", "1", "1");
	
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);//中断优先级分组2--抢占0-4，顺序0-4

	Board_Init();					//初始化LED 
	LEDON;
	SEL_NONE;
	uart_init(115200);		//初始化串口波特率为115200
	SysTick_Config(SystemCoreClock / 1000 * 20);//20ms

	APWR_EN;
		
	printf("\033[2J");// 清除屏幕	
	printf("\r\nmain.c ");printf(__DATE__);printf(__TIME__);printf("\r\n");	
	    

#ifdef USE_USB_OTG_FS  		
 	USBD_Init(&USB_OTG_dev,USB_OTG_FS_CORE_ID,&USR_desc,&CLASS_cb,&USR_cb); 
#endif
#ifdef USE_USB_OTG_HS  	
 	USBD_Init(&USB_OTG_dev,USB_OTG_HS_CORE_ID,&USR_desc,&CLASS_cb,&USR_cb);    
#endif
	
	EVAL_AUDIO_Init();
	
	while(1)
	{  
		if(Divece_STA!=bDeviceState)//状态改变了
		{
			Divece_STA=bDeviceState;
			
			if(bDeviceState!=0)
			{
			LED_YEL;
			}
			else
			{
			LED_RED;
			}
		}

		if(alt_setting_now!=0)
		{
			if(cnt>50)//100x10ms
			{
			cnt=0;
			
			printf("\rfbOk:%d,",fb_success);
			printf("fbNG:%d,",fb_incomplt);
			printf("LOS:%d,",rx_incomplt);
			printf("buf:%d,",data_remain);
			printf("ov:%d,",overrun_counter);
			printf("UD:%d,",underrun_counter);
			printf("   ");
			
			los_cnt++;
			if(los_cnt>60){//60s
			los_cnt=0;
			loscount=rx_incomplt;
			}
			
			}
			if(fb_success<100){rx_incomplt=0;loscount=0;}//lost when start play is ignored.
			if (overrun_counter||underrun_counter) {LED_RED;}//error LED mean out of buffer.
			else {
				if(rx_incomplt>loscount)	{LED_VLT;}//warn LED mean lost package.
				else {
					if(working_samplerate%44100==0) {LED_GRN;}
					else {LED_BLU;}
					}
				}
			
		}
		else
		{
			LED_YEL;
			overrun_counter=0;underrun_counter=0;
			fb_success=0;
			cnt=0;
		}

	} 
}






