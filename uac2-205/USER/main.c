#include "io.h"  
#include "usbd_audio2_core.h"
#include "audio_out.h"
#include "usbd_usr.h"
#include "usb_conf.h" 	
#include "cm_backtrace.h"

extern u8 fb_buf[4];
extern s32 play_speed;

USB_OTG_CORE_HANDLE USB_OTG_dev;
extern vu8 bDeviceState;		//USB连接 情况
u32 cnt=0;

void SysTick_Handler(void)
{
	cnt++;
}

int main(void)
{        
	u8 dstate=0xff;
	u32 los_cnt=0;
	u32 loscount=0;
	
	cm_backtrace_init("1", "1", "1");
	
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);//中断优先级分组2--抢占0-4，顺序0-4

	Board_Init();					//初始化LED 
	BSP_USBLOST;
	SEL_NONE;
	APWR1_EN;
	uart_init(115200);		//初始化串口波特率为115200
	SysTick_Config(SystemCoreClock / 1000 * 20);//20ms

		
	printf("\033[2J");// 清除屏幕	
	printf("\r\nmain.c ");printf(__DATE__);printf(__TIME__);printf("\r\n");	
	    
 	USBD_Init(&USB_OTG_dev,USB_OTG_HS_CORE_ID,&USR_desc,&CLASS_cb,&USR_cb);    
	
	EVAL_AUDIO_Init();
	//APWR2_EN;
	
	while(1)
	{  
		if(dstate!=bDeviceState)//状态改变了
		{
			dstate=bDeviceState;
			
			switch (bDeviceState)
{
	case 0 :
			BSP_USBLOST;
	
		break;
	case 1 :
			BSP_USBREADY;
			
	APWR2_EN;
		break;
	default:
	BSP_USBSLEEP;
		break;
}
			
		}

		if(alt_setting_now!=0)
		{
			if(cnt>50)//100x10ms
			{
			cnt=0;
			
			printf("\rTok:%d,",fb_success);
			printf("TNG:%d,",fb_incomplt);
			printf("LOS:%d,",rx_incomplt);
			printf("ov:%d,",overrun_counter);
			printf("UD:%d,",underrun_counter);
			printf("buf:%d,",data_remain);
			printf("s:%d,",play_speed);
			printf("fb:%X,%X,%X,%X",fb_buf[0],fb_buf[1],fb_buf[2],fb_buf[3]);
			
			
			los_cnt++;
			if(los_cnt>60){//60s
			los_cnt=0;
			loscount=rx_incomplt;
			}
			
			}
			if(fb_success<100){rx_incomplt=0;loscount=0;}//lost when start play is ignored.
			if (overrun_counter||underrun_counter) {BSP_AUDIOERR;}//error LED mean out of buffer.
			else {
				if(rx_incomplt>loscount)	{BSP_AUDIOWARN;}//warn LED mean lost package.
				else {
					
					}
				}
			
		}
		else
		{
			BSP_AUDIOSTOP;
			overrun_counter=0;underrun_counter=0;
			fb_success=0;
			cnt=0;
		}

	} 
}






