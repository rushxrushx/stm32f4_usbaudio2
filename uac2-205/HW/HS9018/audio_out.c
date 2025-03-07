#include "io.h"
#include "usbd_audio2_core.h"
#include "audio_out.h"
 
vu8 audiostatus=0;							//bit0:0,暂停播放;1,继续播放  
vu32 working_samplerate=44100;	//当前采样频率 
vu32 Play_ptr=0;								//即将播放的音频帧缓冲编号
vu32 Write_ptr=0;							//当前保存到的音频缓冲编号 
u32 underrun_counter=0;
u32 const i2s_BUFSIZEMX=4000;	
vu32 i2s_BUFSIZE=4000;								
u32 i2s_buf[i2s_BUFSIZEMX+2]; 	//音频缓冲
void audio_i2s_dma_callback(void);


#define I2SCFGR_CLEAR_MASK        ((uint16_t)0xF040)
void I2S_Init_E(SPI_TypeDef* SPIx, I2S_InitTypeDef* I2S_InitStruct )
{
  uint16_t tmpreg = 0, i2sdiv = 2, i2sodd = 0, packetlength = 1;
  uint32_t tmp = 0 ;
u32 i2sclk;
  
  /* Check the I2S parameters */
  assert_param(IS_SPI_23_PERIPH(SPIx));
  assert_param(IS_I2S_MODE(I2S_InitStruct->I2S_Mode));
  assert_param(IS_I2S_STANDARD(I2S_InitStruct->I2S_Standard));
  assert_param(IS_I2S_DATA_FORMAT(I2S_InitStruct->I2S_DataFormat));
  assert_param(IS_I2S_MCLK_OUTPUT(I2S_InitStruct->I2S_MCLKOutput));
  assert_param(IS_I2S_AUDIO_FREQ(I2S_InitStruct->I2S_AudioFreq));
  assert_param(IS_I2S_CPOL(I2S_InitStruct->I2S_CPOL));  

////双时钟控制  
	if((I2S_InitStruct->I2S_AudioFreq)%44100 ==0){
	SEL_441;	
	i2sclk=45158400;
	} 
	else {
	SEL_480;
	i2sclk=49152000;
	}

/*----------------------- SPIx I2SCFGR & I2SPR Configuration -----------------*/
  /* Clear I2SMOD, I2SE, I2SCFG, PCMSYNC, I2SSTD, CKPOL, DATLEN and CHLEN bits */
  SPIx->I2SCFGR &= I2SCFGR_CLEAR_MASK; 
  SPIx->I2SPR = 0x0002;
  
  /* Get the I2SCFGR register value */
  tmpreg = SPIx->I2SCFGR;

    /* Check the frame length (For the Prescaler computing) *******************/
    if(I2S_InitStruct->I2S_DataFormat == I2S_DataFormat_16b)
    {
      /* Packet length is 16 bits */
      packetlength = 1;
    }
    else
    {
      /* Packet length is 32 bits */
      packetlength = 2;
    }
    
    /* Compute the Real divider depending on the MCLK output state, with a floating point */
    if(I2S_InitStruct->I2S_MCLKOutput == I2S_MCLKOutput_Enable)
    {
      /* MCLK output is enabled */
      tmp = (uint16_t)((((i2sclk / 256)  / I2S_InitStruct->I2S_AudioFreq)) + 0);
    }
    else
    {
      /* MCLK output is disabled */
      tmp = (uint16_t)((((i2sclk / (32 * packetlength)) / I2S_InitStruct->I2S_AudioFreq)) + 0);
    }
    
    /* Remove the flatting point */
    //tmp = tmp / 1;  
      
    /* Check the parity of the divider */
    i2sodd = 0;
   
    /* Compute the i2sdiv prescaler */
    i2sdiv = (uint16_t)((tmp - i2sodd) / 2);
   
    /* Get the Mask for the Odd bit (SPI_I2SPR[8]) register */
   // i2sodd = (uint16_t) (i2sodd << 8);


  /* Test if the divider is 1 or 0 or greater than 0xFF */
 // if ((i2sdiv < 2) || (i2sdiv > 0xFF))
  //{
    /* Set the default values */
  //  i2sdiv = 2;
  //  i2sodd = 0;
 // }

  /* Write to SPIx I2SPR register the computed value */
  SPIx->I2SPR = (uint16_t)((uint16_t)i2sdiv | (uint16_t)(i2sodd | (uint16_t)I2S_InitStruct->I2S_MCLKOutput));
 
  /* Configure the I2S with the SPI_InitStruct values */
  tmpreg |= (uint16_t)((uint16_t)SPI_I2SCFGR_I2SMOD | (uint16_t)(I2S_InitStruct->I2S_Mode | \
                  (uint16_t)(I2S_InitStruct->I2S_Standard | (uint16_t)(I2S_InitStruct->I2S_DataFormat | \
                  (uint16_t)I2S_InitStruct->I2S_CPOL))));
 
  /* Write to SPIx I2SCFGR */  
  SPIx->I2SCFGR = tmpreg;
}	




// FS usb,401 chip,
// spi2 is avliable
#ifdef USE_USB_OTG_FS 
void I2S_GPInit(void)
{
	GPIO_InitTypeDef  GPIO_InitStructure;

    RCC_I2SCLKConfig(RCC_I2S2CLKSource_Ext);/////必须在使用前设置
	  
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA|RCC_AHB1Periph_GPIOB|RCC_AHB1Periph_GPIOC, ENABLE);
     
	// Connect pins to I2S peripheral 
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource12, GPIO_AF_SPI2);
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource13, GPIO_AF_SPI2);
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource15, GPIO_AF_SPI2);
	GPIO_PinAFConfig(GPIOC, GPIO_PinSource9, GPIO_AF_SPI2);//PC9->ckin
	
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_15;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
	GPIO_Init(GPIOC, &GPIO_InitStructure);	
   
}

//I2S_Standard_Phillips,标准I2S;
//I2S_Standard_MSB,左对齐;
//I2S_Standard_LSB,右对齐;
//I2S_DataFormat_16b,16位;
//I2S_DataFormat_16bextended,16in32;
//I2S_DataFormat_24b,24位;
//I2S_DataFormat_32b,32位.
void I2S_Reconf(uint32_t samplerate)
{
	I2S_InitTypeDef I2S_InitStructure;
	
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2, ENABLE);//使能SPI2时钟
	RCC_APB1PeriphResetCmd(RCC_APB1Periph_SPI2,ENABLE); //复位SPI2
	RCC_APB1PeriphResetCmd(RCC_APB1Periph_SPI2,DISABLE);//结束复位
  
	I2S_InitStructure.I2S_Mode=I2S_Mode_MasterTx;
	I2S_InitStructure.I2S_Standard=I2S_Standard_Phillips;
	I2S_InitStructure.I2S_DataFormat=I2S_DataFormat_32b;
	I2S_InitStructure.I2S_MCLKOutput=I2S_MCLKOutput_Disable;//主时钟输出
	I2S_InitStructure.I2S_AudioFreq=samplerate;
	I2S_InitStructure.I2S_CPOL=I2S_CPOL_Low;//空闲状态时钟电平
	I2S_Init_E(SPI2,&I2S_InitStructure);//初始化IIS
 
	SPI_I2S_DMACmd(SPI2,SPI_I2S_DMAReq_Tx,ENABLE);//SPI2 TX DMA请求使能.
	I2S_Cmd(SPI2,ENABLE);//SPI2 I2S EN使能.
	
}


// spi2 DMA1_Stream4
#define AUDIO_I2S_DMA_DMAX     RCC_AHB1Periph_DMA1
#define AUDIO_I2S_DMA_STREAM    DMA1_Stream4
#define AUDIO_I2S_DMA_CHANNEL   DMA_Channel_0
#define AUDIO_I2S_DMA_IRQ       DMA1_Stream4_IRQn
#define AUDIO_I2S_DMA_IT_TC     DMA_IT_TCIF4
#define AUDIO_I2S_SPI_ADDR 		(u32)&SPI2->DR
#define AUDIO_I2S_SPI_SPIX		RCC_APB1Periph_SPI2

//DMA1_Stream4中断服务函数
void DMA1_Stream4_IRQHandler(void)
{      
	if(DMA_GetITStatus(AUDIO_I2S_DMA_STREAM,AUDIO_I2S_DMA_IT_TC)==SET)////DMA1_Stream4,传输完成标志
	{ 
		DMA_ClearITPendingBit(AUDIO_I2S_DMA_STREAM,AUDIO_I2S_DMA_IT_TC);

		audio_i2s_dma_callback();
	}   											 
}

	
#endif

// 205 chip with usb3300 HS usb
// spi2 not aviliable,must change to spi3 
#ifdef USE_USB_OTG_HS 
void I2S_GPInit(void)//spi3 gpio
{
	GPIO_InitTypeDef  GPIO_InitStructure;

    RCC_I2SCLKConfig(RCC_I2S2CLKSource_Ext);/////必须在使用前设置
	  
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA|RCC_AHB1Periph_GPIOB|RCC_AHB1Periph_GPIOC, ENABLE);
     
	
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;

	/* Connect pins to I2S peripheral  */
	GPIO_PinAFConfig(GPIOA, GPIO_PinSource15, GPIO_AF_SPI3);
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource3, GPIO_AF_SPI3);
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource4, GPIO_AF_I2S3ext);
//	GPIO_PinAFConfig(GPIOB, GPIO_PinSource5, GPIO_AF_SPI3);
   GPIO_PinAFConfig(GPIOC, GPIO_PinSource9, GPIO_AF_SPI2);//ckin
   GPIO_PinAFConfig(GPIOC, GPIO_PinSource12, GPIO_AF_SPI3);//pc12->spi3 out

	/* I2S3S_WS */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_15;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	/* I2S3_CK, I2S3_SD, I2S3ext_SD */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3 | GPIO_Pin_4 ;
	GPIO_Init(GPIOB, &GPIO_InitStructure);


   GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9 | GPIO_Pin_12;
   GPIO_Init(GPIOC, &GPIO_InitStructure);
   
} 

//I2S_Standard_Phillips,标准I2S;
//I2S_Standard_MSB,左对齐;
//I2S_Standard_LSB,右对齐;
//I2S_DataFormat_16b,16位;
//I2S_DataFormat_16bextended,16in32;
//I2S_DataFormat_24b,24位;
//I2S_DataFormat_32b,32位.
void I2S_Reconf(uint32_t samplerate)
{
	I2S_InitTypeDef I2S_InitStructure;
	
	//RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI3, ENABLE);//使能SPIx时钟
	//RCC_APB1PeriphResetCmd(RCC_APB1Periph_SPI3,ENABLE); //复位SPIx
	RCC_APB1PeriphResetCmd(RCC_APB1Periph_SPI3,DISABLE);//结束复位
  
	I2S_InitStructure.I2S_Mode=I2S_Mode_MasterTx;
	I2S_InitStructure.I2S_Standard=I2S_Standard_Phillips;
	I2S_InitStructure.I2S_DataFormat=I2S_DataFormat_32b;
	I2S_InitStructure.I2S_MCLKOutput=I2S_MCLKOutput_Disable;//主时钟输出
	I2S_InitStructure.I2S_AudioFreq=samplerate;
	I2S_InitStructure.I2S_CPOL=I2S_CPOL_Low;//空闲状态时钟电平
	I2S_Init_E(SPI3,&I2S_InitStructure);//初始化IIS

 
	SPI_I2S_DMACmd(SPI3,SPI_I2S_DMAReq_Tx,ENABLE);//SPIx TX DMA请求使能.
	I2S_Cmd(SPI3,ENABLE);//I2Sx 使能.
	
}

// SPI3 DMA1_Stream7
#define AUDIO_I2S_DMA_DMAX     RCC_AHB1Periph_DMA1
#define AUDIO_I2S_DMA_STREAM    DMA1_Stream7
#define AUDIO_I2S_DMA_CHANNEL   DMA_Channel_0
#define AUDIO_I2S_DMA_IRQ       DMA1_Stream7_IRQn
#define AUDIO_I2S_DMA_IT_TC     DMA_IT_TCIF7
#define AUDIO_I2S_SPI_ADDR 		(u32)&SPI3->DR
#define AUDIO_I2S_SPI_SPIX		RCC_APB1Periph_SPI3


//DMA1_Stream7中断服务函数
void DMA1_Stream7_IRQHandler(void)
{      
	if(DMA_GetITStatus(AUDIO_I2S_DMA_STREAM,AUDIO_I2S_DMA_IT_TC)==SET)////DMA1_Stream7,传输完成标志
	{ 
		DMA_ClearITPendingBit(AUDIO_I2S_DMA_STREAM,AUDIO_I2S_DMA_IT_TC);

		audio_i2s_dma_callback();
	}   											 
}	
#endif


//I2S DMA IRQ配置
void I2S_DMA_irqInit()
{  
	NVIC_InitTypeDef   NVIC_InitStructure;
		
	NVIC_InitStructure.NVIC_IRQChannel = AUDIO_I2S_DMA_IRQ; 
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;//抢占优先级0-4
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;//顺序优先级0-4
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;//使能外部中断通道
	NVIC_Init(&NVIC_InitStructure);//配置

} 	

//I2S2 DMA配置
void I2S2_DMA_Reconf()
{  
u32 i=0;
	DMA_InitTypeDef  DMA_InitStructure;
	
	//RCC_AHB1PeriphClockCmd(AUDIO_I2S_DMA_DMAX,ENABLE);//DMA1时钟使能 
	//RCC_AHB1PeriphResetCmd(AUDIO_I2S_DMA_DMAX, ENABLE);//start reset
	//RCC_AHB1PeriphResetCmd(AUDIO_I2S_DMA_DMAX, DISABLE);//end reset
	
	DMA_DeInit(AUDIO_I2S_DMA_STREAM);
	while (DMA_GetCmdStatus(AUDIO_I2S_DMA_STREAM) != DISABLE)
		{i++;if (i>100000){printf("init DMA timeout!\r\n");}}//等待DMA可配置 

  DMA_InitStructure.DMA_Channel = AUDIO_I2S_DMA_CHANNEL;  //通道0 SPI3_TX通道 
  DMA_InitStructure.DMA_PeripheralBaseAddr = AUDIO_I2S_SPI_ADDR;//外设地址
  DMA_InitStructure.DMA_Memory0BaseAddr = (u32) &(i2s_buf[Play_ptr]);//存储器地址
  DMA_InitStructure.DMA_DIR = DMA_DIR_MemoryToPeripheral;//存储器到外设模式
  DMA_InitStructure.DMA_BufferSize = 2;
  DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;//外设非增量模式
  DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;//存储器增量模式
  DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;//外设数据长度:16位
  DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;//存储器数据长度：32位 
  DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;// 非循环模式 
  DMA_InitStructure.DMA_Priority = DMA_Priority_High;//高优先级
  DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Enable; //使用FIFO模式        
  DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_HalfFull;
  DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;//突发关闭
  DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;//突发关闭
  DMA_Init(AUDIO_I2S_DMA_STREAM, &DMA_InitStructure);//初始化DMA Stream
 
  DMA_ITConfig(AUDIO_I2S_DMA_STREAM,DMA_IT_TC,ENABLE);//开启传输完成中断

}  


//DMA开始播放
void EVAL_AUDIO_Play(void)
{
	I2S2_DMA_Reconf();
	//更新 DMA 读取入口地址
	DMA_MemoryTargetConfig(AUDIO_I2S_DMA_STREAM, (u32) &(i2s_buf[Play_ptr]) ,DMA_Memory_0);
	//设置好数据量
	DMA_SetCurrDataCounter(AUDIO_I2S_DMA_STREAM, 4);//内部精度32位，不受usb控制
	//重启DMA
	DMA_Cmd(AUDIO_I2S_DMA_STREAM,ENABLE);
	I2S_Reconf(working_samplerate);
	DAC_EN;
	//LEDON;
	audiostatus=1;
}
 
 
//DMA停止播放
void EVAL_AUDIO_Stop(void)
{
I2S_Cmd(SPI3,DISABLE);
DMA_Cmd(AUDIO_I2S_DMA_STREAM,DISABLE);

	RCC_APB1PeriphResetCmd(RCC_APB1Periph_SPI3,ENABLE); //复位SPIx
	//RCC_APB1PeriphClockCmd(AUDIO_I2S_SPI_SPIX, DISABLE);//停止SPIx时钟，无需等待
	//RCC_AHB1PeriphClockCmd(AUDIO_I2S_DMA_DMAX, DISABLE);//停止DMAx时钟，无需等待
	DAC_DIS;
	SEL_NONE;
	//LEDOFF;
	audiostatus=0;
	if (alt_setting_now==0){overrun_counter=0;underrun_counter=0;}
}


//音频数据I2S DMA传输回调函数
void audio_i2s_dma_callback(void) 
{ 
u32 next_Playptr;

	DMA_Cmd(AUDIO_I2S_DMA_STREAM,DISABLE);//关闭DMA才能修改

	//算下一个数据帧
	if (Play_ptr < i2s_BUFSIZE ) next_Playptr = Play_ptr + 2;
	else  next_Playptr = 0;          //循环回头部
    
	if( next_Playptr == Write_ptr)	//撞尾了
	{ 
		EVAL_AUDIO_Stop();
		underrun_counter++;
		return;
	}else
	{
		Play_ptr = next_Playptr;//可以开开心心去下一个数据帧
	}

	//更新 DMA 读取入口地址
	DMA_MemoryTargetConfig(AUDIO_I2S_DMA_STREAM, (u32) &(i2s_buf[Play_ptr]) ,DMA_Memory_0);

	//设置好数据量
	DMA_SetCurrDataCounter(AUDIO_I2S_DMA_STREAM, 4);//内部精度32位，不受usb控制

	//重启DMA
	DMA_Cmd(AUDIO_I2S_DMA_STREAM,ENABLE);

}

//配置音频接口
//AudioFreq:音频采样率
uint32_t EVAL_AUDIO_Init()
{ 
	I2S_GPInit();
	
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI3, ENABLE);//使能SPI2时钟
	RCC_APB1PeriphResetCmd(RCC_APB1Periph_SPI3,ENABLE); //复位SPI2
	RCC_APB1PeriphResetCmd(RCC_APB1Periph_SPI3,DISABLE);//结束复位
	
	I2S_DMA_irqInit();
	
	RCC_AHB1PeriphClockCmd(AUDIO_I2S_DMA_DMAX,ENABLE);//DMA1时钟使能 
	RCC_AHB1PeriphResetCmd(AUDIO_I2S_DMA_DMAX, ENABLE);//start reset
	RCC_AHB1PeriphResetCmd(AUDIO_I2S_DMA_DMAX, DISABLE);//end reset
	
	
	
	EVAL_AUDIO_Stop();
	return 0; 
}


















