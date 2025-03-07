#define	__IO	volatile
#include "usbd_audio2_core.h"
#include "audio_out.h"
#include "io.h"


#define bitdepth 32
#define bpf (bitdepth/8*2) //byte per frame   帧字节数
#define bpfs (bitdepth/8)  //sub frame size  单声道字节数

// 接收数据包临时缓冲区，相当于103的PMA
u32 IsocOutBuff [AUDIO_OUT_PKTSIZE / 4];   //以整数为单位，拒绝左右声道错乱
                                           //AUDIO_OUT_PKTSIZE u8 , 这里是u32 所以除以4
u32 overrun_counter=0;
u32 fb_success=0;
u32 fb_incomplt=0;
u32 rx_incomplt=0;
vu32 data_remain;
s32 play_speed=0;
u8 fb_buf[4];
vu32 alt_setting_now=0;
u8 audioIsMute=0;
u8 audioVol=100; 

/* Main Buffer for Audio Control requests transfers and its relative variables */
u8  AudioCtl[64];
u8  AudioCtlReq = 0;
u8  AudioCtlCmd = 0;
u32 AudioCtlLen = 0;
u8  AudioCtlCS  = 0;
u8  AudioCtlUnit = 0;


/* USB AUDIO device Configuration Descriptor */
static uint8_t usbd_audio_CfgDesc[AUDIO_CONFIG_DESC_SIZE] =
{
  /* Configuration 1 */
  0x09,                                 /* bLength */
  USB_CONFIGURATION_DESCRIPTOR_TYPE,    /* bDescriptorType */
  LOBYTE(AUDIO_CONFIG_DESC_SIZE),       /* wTotalLength */
  HIBYTE(AUDIO_CONFIG_DESC_SIZE),      
  0x02,                                 /* bNumInterfaces */
  0x01,                                 /* bConfigurationValue */
  0x00,                                 /* iConfiguration */
  0xC0,                                 /* bmAttributes BUS Powered*/
  0x32,                                 /* bMaxPower = 100 mA*/
  /* 09 byte*/
  
  0x08,
  0x0B,
  0x00,
  0x02,
  0x01,
  0x00,
  0x20,
  0x00,
  /* 08 byte */
  
  /* Standard AC Interface Descriptor */
  AUDIO_INTERFACE_DESC_SIZE,            /* bLength */
  USB_INTERFACE_DESCRIPTOR_TYPE,        /* bDescriptorType */
  0x00,                                 /* bInterfaceNumber */
  0x00,                                 /* bAlternateSetting */
  0x00,                                 /* bNumEndpoints */
  USB_DEVICE_CLASS_AUDIO,               /* bInterfaceClass */
  AUDIO_SUBCLASS_AUDIOCONTROL,          /* bInterfaceSubClass */
  AUDIO_IP_VERSION_02_00,               /* bInterfaceProtocol */
  0x00,                                 /* iInterface */
  /* 09 byte*/
  
  /* Class-specific AC Interface Descriptor */
  AUDIO_INTERFACE_DESC_SIZE,            /* bLength */
  AUDIO_INTERFACE_DESCRIPTOR_TYPE,      /* bDescriptorType */
  AUDIO_CONTROL_HEADER,                 /* bDescriptorSubtype */
  0x00,                                 /* bcdADC */
  0x02,                                 /* 2.00 */
  AUDIO_HEADSET,                        /* bCategory */
  0x40,                                 /* wTotalLength */
  0x00,
  0x00,                                 /* bmControls */
  /* 09 byte*/
  
  /* USB HeadSet Clock Source Descriptor */
  AUDIO_20_CLK_SOURCE_DESC_SIZE,        /* bLength */
  AUDIO_INTERFACE_DESCRIPTOR_TYPE,      /* bDescriptorType */
  AUDIO_CONTROL_CLOCK_SOURCE,           /* bDescriptorSubtype */
  AUDIO_CLK_ID,                         /* bClockID */
  0x03,                                 /* bmAttributes */
  0x07,                                 /* bmControls TODO */
  0x00,                                 /* bAssocTerminal */
  0x00,                                 /* iClockSource */
  /* 08 byte*/
  
  /* USB HeadSet Input Terminal Descriptor */
  AUDIO_20_IT_DESC_SIZE,                /* bLength */
  AUDIO_INTERFACE_DESCRIPTOR_TYPE,      /* bDescriptorType */
  AUDIO_CONTROL_INPUT_TERMINAL,         /* bDescriptorSubtype */
  AUDIO_IT_ID,                          /* bTerminalID */
  0x01,                                 /* wTerminalType AUDIO_TERMINAL_USB_STREAMING   0x0201 */
  0x01,
  0x00,                                 /* bAssocTerminal */
  AUDIO_CLK_ID,                         /* bCSourceID*/
  0x02,                                 /* bNrChannels */
  0x03,                                 /* wChannelConfig 0x00000003  Stereo */
  0x00,
  0x00,
  0x00,
  0x00,                                 /* iChannelNames */
  0x00,                                 /* bmControls */
  0x00,
  0x00,                                 /* iTerminal */
  /* 17 byte*/
  
  /* USB HeadSet Audio Feature Unit Descriptor */
  0x12,                                 /* bLength */
  AUDIO_INTERFACE_DESCRIPTOR_TYPE,      /* bDescriptorType */
  AUDIO_CONTROL_FEATURE_UNIT,           /* bDescriptorSubtype */
  AUDIO_FU_ID,                          /* bUnitID */
  AUDIO_IT_ID,                          /* bSourceID */
  AUDIO_20_CTL_MUTE(CONTROL_BITMAP_PROG)/* bmaControls(0) */
 // | AUDIO_20_CTL_VOLUME(CONTROL_BITMAP_PROG),
 ,
  0x00,
  0x00,
  0x00,
  0x00,                                 /* bmaControls(1) */
  0x00,
  0x00,
  0x00,
  0x00,                                 /* bmaControls(2) */
  0x00,
  0x00,
  0x00,
  0x00,                                 /* iFeature */
  /* 18 byte*/
  
  /*USB HeadSet Output Terminal Descriptor */
  AUDIO_20_OT_DESC_SIZE,      			/* bLength */
  AUDIO_INTERFACE_DESCRIPTOR_TYPE,      /* bDescriptorType */
  AUDIO_CONTROL_OUTPUT_TERMINAL,        /* bDescriptorSubtype */
  AUDIO_OT_ID,                          /* bTerminalID */
  0x01,                                 /* wTerminalType  0x0301: Speaker, 0x0302: Headphone*/
  0x03,
  0x00,                                 /* bAssocTerminal */
  AUDIO_FU_ID,                          /* bSourceID */
  AUDIO_CLK_ID,                         /* bCSourceID */
  0x00,                                 /* bmControls */
  0x00,
  0x00,                                 /* iTerminal */
  /* 12 byte*/
  
  /* USB HeadSet Standard AS Interface Descriptor - Audio Streaming Zero Bandwidth */
  /* Interface 1, Alternate Setting 0                                              */
  AUDIO_INTERFACE_DESC_SIZE,  			/* bLength */
  USB_INTERFACE_DESCRIPTOR_TYPE,        /* bDescriptorType */
  0x01,                                 /* bInterfaceNumber */
  0x00,                                 /* bAlternateSetting */
  0x00,                                 /* bNumEndpoints */
  USB_DEVICE_CLASS_AUDIO,               /* bInterfaceClass */
  AUDIO_SUBCLASS_AUDIOSTREAMING,        /* bInterfaceSubClass */
  AUDIO_IP_VERSION_02_00,               /* bInterfaceProtocol */
  0x00,                                 /* iInterface */
  /* 09 byte*/
  
  /* USB Speaker Standard AS Interface Descriptor - Audio Streaming Operational */
  /* Interface 1, Alternate Setting 1                                           */
  AUDIO_INTERFACE_DESC_SIZE,  			/* bLength */
  USB_INTERFACE_DESCRIPTOR_TYPE,        /* bDescriptorType */
  0x01,                                 /* bInterfaceNumber */
  0x01,                                 /* bAlternateSetting */
  0x02,                                 /* bNumEndpoints */
  USB_DEVICE_CLASS_AUDIO,               /* bInterfaceClass */
  AUDIO_SUBCLASS_AUDIOSTREAMING,        /* bInterfaceSubClass */
  AUDIO_IP_VERSION_02_00,               /* bInterfaceProtocol */
  0x00,                                 /* iInterface */
  /* 09 byte*/
  
  /* USB HeadSet Audio Streaming Interface Descriptor */
  AUDIO_20_STREAMING_INTERFACE_DESC_SIZE,  /* bLength */
  AUDIO_INTERFACE_DESCRIPTOR_TYPE,      /* bDescriptorType */
  AUDIO_STREAMING_GENERAL,              /* bDescriptorSubtype */
  AUDIO_IT_ID,                          /* 0x01: bTerminalLink */
  0x00,                                 /* bmControls */
  AUDIO_FORMAT_TYPE_I,                  /* bFormatType */
  0x01,                                 /* bmFormats PCM */
  0x00,
  0x00,
  0x00,
  0x02,                                 /* bNrChannels */
  0x03,                                 /* bmChannelConfig */
  0x00,
  0x00,
  0x00,
  0x00,                                 /* iChannelNames */
  /* 16 byte*/
  
  /* USB Speaker Audio Type I Format Interface Descriptor */
  0x06,                                 /* bLength */
  AUDIO_INTERFACE_DESCRIPTOR_TYPE,      /* bDescriptorType */
  AUDIO_STREAMING_FORMAT_TYPE,          /* bDescriptorSubtype */
  AUDIO_FORMAT_TYPE_I,                  /* bFormatType */
  bpfs,                      /* bSubslotSize */
  bitdepth,                          /* bBitResolution */
  /* 6 byte*/
  
  /* Endpoint 1 - Standard Descriptor */
  AUDIO_20_STANDARD_ENDPOINT_DESC_SIZE, /* bLength */
  USB_ENDPOINT_DESCRIPTOR_TYPE,         /* bDescriptorType */
  AUDIO_OUT_EP,                         /* bEndpointAddress 3 out endpoint for Audio */
  USB_ENDPOINT_TYPE_ASYNCHRONOUS,           /* bmAttributes */
  (AUDIO_OUT_PKTSIZE + 0)&0xff, ((AUDIO_OUT_PKTSIZE + 0)>>8)&0xff,     /* wMaxPacketSize in Bytes (Freq(Samples)*2(Stereo)*2(HalfWord)) */
  0x01,                                 /* bInterval */
  /* 07 byte*/
  
  /* Endpoint - Audio Streaming Descriptor*/
  AUDIO_20_STREAMING_ENDPOINT_DESC_SIZE,/* bLength */
  AUDIO_ENDPOINT_DESCRIPTOR_TYPE,       /* bDescriptorType */
  AUDIO_ENDPOINT_GENERAL,               /* bDescriptor */
  0x00,                                 /* bmAttributes */
  0x00,                                 /* bmControls */
  0x00,                                 /* bLockDelayUnits */
  0x00,                                 /* wLockDelay */
  0x00,
  /* 08 byte*/
  
  
    /* AS Isochronous Feedback Endpoint Descriptor */
    0x07,                           /* bLength */       
    0x05,                           /* bDescriptorType: ENDPOINT */
    AUDIO_IN_EP,       /* bEndpointAddress (D3:0 - EP no. D6:4 - reserved 0. D7 - 0:out, 1:in) */
    0x11,                           /* bmAttributes (bitmap)  */ 
    4, 0,               /* wMaxPacketSize 4 bytes */
    0x01,                           /* bInterval */
	//added 7 bytes
} ;

//Get Layout 3 parameter block
static uint8_t sample_desc[] = {
	SAMPLE_FREQ_NUM(6),                       /* wNumSubRanges */
	
	SAMPLE_FREQ_4B(I2S_AudioFreq_44k),        /* dMIN */
	SAMPLE_FREQ_4B(I2S_AudioFreq_44k),        /* dMAX */
	SAMPLE_FREQ_4B(0x00),                     /* dRES */
	
	SAMPLE_FREQ_4B(I2S_AudioFreq_48k),        /* dMIN */
	SAMPLE_FREQ_4B(I2S_AudioFreq_48k),        /* dMAX */
	SAMPLE_FREQ_4B(0x00),                     /* dRES */
	
	SAMPLE_FREQ_4B((uint32_t)88200),        /* dMIN */
	SAMPLE_FREQ_4B((uint32_t)88200),        /* dMAX */
	SAMPLE_FREQ_4B(0x00),                     /* dRES */
	
	SAMPLE_FREQ_4B(I2S_AudioFreq_96k),        /* dMIN */
	SAMPLE_FREQ_4B(I2S_AudioFreq_96k),        /* dMAX */
	SAMPLE_FREQ_4B(0x00),                     /* dRES */
	
	SAMPLE_FREQ_4B( (uint32_t)176400),        /* dMIN */
	SAMPLE_FREQ_4B( (uint32_t)176400),        /* dMAX */
	SAMPLE_FREQ_4B(0x00),                     /* dRES */
	
	SAMPLE_FREQ_4B(I2S_AudioFreq_192k),        /* dMIN */
	SAMPLE_FREQ_4B(I2S_AudioFreq_192k),        /* dMAX */
	SAMPLE_FREQ_4B(0x00),                     /* dRES */
	
//	SAMPLE_FREQ_4B( (uint32_t)352800),        			/* dMIN */
//	SAMPLE_FREQ_4B( (uint32_t)352800),        			/* dMAX */
//	SAMPLE_FREQ_4B(0x00),                     /* dRES */
	
//	SAMPLE_FREQ_4B( (uint32_t)384000),        			/* dMIN */
//	SAMPLE_FREQ_4B( (uint32_t)384000),        			/* dMAX */
//	SAMPLE_FREQ_4B(0x00),                     /* dRES */
	
};



/******************************************************************************
     AUDIO Class requests management
******************************************************************************/
static void AUDIO_Req_ClockSource(void *pdev, USB_SETUP_REQ *req)
{
uint8_t req_type = 	req->bmRequest; /* Set the request type. */
uint8_t cmd = 		req->bRequest;  		/* Set the request value */
//uint8_t len = 		(uint8_t)req->wLength;      /* data length */
//uint8_t unit = 		HIBYTE(req->wIndex);       /* target unit */
//uint8_t index = 	req->wIndex & 0xf;		/* endpoint number */
uint8_t CS = 	HIBYTE(req->wValue);         /* control selector (high byte) */
//uint8_t CN = 	LOBYTE(req->wValue);         /* control number (low byte) */  

    switch (CS) {
        case CS_CONTROL_UNDEFINED:
            USBD_CtlError (pdev, req);
            break;
        
        case CS_SAM_FREQ_CONTROL:	//采样率获取/设置
            switch (cmd) {
                case AUDIO_REQ_CUR:	//获取工作采样率

                    if (req_type & AUDIO_REQ_GET_MASK) {
                        //Get Layout 3 parameter block
						
						AudioCtl[0] = (uint8_t)(working_samplerate);
						AudioCtl[1] = (uint8_t)(working_samplerate >> 8);
						AudioCtl[2] = (uint8_t)(working_samplerate >> 16);
						AudioCtl[3] = (uint8_t)(working_samplerate >> 24);
				
                        USBD_CtlSendData(pdev, AudioCtl, 4 );
                    
					}
					else      //Set //设置采样率 
					{
						/* Set the global variables indicating current request and its length 
						to the function usbd_audio_EP0_RxReady() which will process the request */
						AudioCtlReq = AUDIO_REQ_INTERFACE;  
						AudioCtlUnit = AUDIO_CLK_ID;  /* Set the request target unit */
						AudioCtlCS  = CS_SAM_FREQ_CONTROL;
						
						USBD_CtlPrepareRx (pdev, AudioCtl, 4);
								
                    }
                    break;
                
                case AUDIO_REQ_RANGE:		//获取设备能力
                    if (req_type & AUDIO_REQ_GET_MASK) {

                        USBD_CtlSendData(pdev, sample_desc, req->wLength);
						
                    } else {
                        //Set
                    }
                    break;
                   
                default:
                    USBD_CtlError (pdev, req);
            }
            break;
        
        case CS_CLOCK_VALID_CONTROL:
            if ((req_type & AUDIO_REQ_GET_MASK) &&
                (cmd == AUDIO_REQ_CUR)) {
                // Only Get CUR Request, Layout 1 parameter block
                AudioCtl[0] = (uint8_t)(1);
				
                USBD_CtlSendData(pdev, AudioCtl,1);
				
            } else {
                USBD_CtlError (pdev, req);
            }
            break;
        
        default:
            USBD_CtlError (pdev, req);
            return;
    }
}

static void AUDIO_Req_FeatureUnit(void *pdev, USB_SETUP_REQ *req)
{
  uint8_t bCS = CS_ID(req->wValue);
  uint8_t bCN = CN_ID(req->wValue);
  
  //memset(AudioCtl, 0, sizeof(AudioCtl));
  if (bCS == AUDIO_CONTROL_VOLUME && bCN == 0) {
    //Layout 2 Parameter
    switch (req->bRequest) {
        case AUDIO_REQ_CUR:
          if (req->bmRequest & AUDIO_REQ_GET_MASK) {
            //GET
            AudioCtl[0] = 100;
            USBD_CtlSendData (pdev, AudioCtl, req->wLength);
          } 
		  else 	//SET
		  {
            /* Prepare the reception of the buffer over EP0 */
            USBD_CtlPrepareRx (pdev, AudioCtl, req->wLength);
    
            /* Set the global variables indicating current request and its length 
            to the function usbd_audio_EP0_RxReady() which will process the request */
            AudioCtlCmd = AUDIO_REQ_SET_CUR;     /* Set the request value */
            AudioCtlLen = req->wLength;          /* Set the request data length */
            AudioCtlCS  = bCS;
            AudioCtlUnit = HIBYTE(req->wIndex);  /* Set the request target unit */            
          }
          break;

        case AUDIO_REQ_RANGE:
          if (req->bmRequest & AUDIO_REQ_GET_MASK) {
            //GET
            AudioCtl[0] = 1;    //wNumSubRanges
            AudioCtl[1] = 0;
            AudioCtl[2] = 0;    //wMIN(1)
            AudioCtl[3] = 0;
            AudioCtl[4] = 100;  //wMAX(1)
            AudioCtl[5] = 0;
            AudioCtl[6] = 1;    //wRES(1)
            AudioCtl[7] = 0;
            USBD_CtlSendData (pdev, AudioCtl, req->wLength);
          } else {
            //SET
            USBD_CtlError (pdev, req);
          }
          break;

        default:
          USBD_CtlError (pdev, req);
          return;
    }
  } else if (bCS == AUDIO_CONTROL_MUTE && bCN == 0) {
    //Layout 2 Parameter
    switch (req->bRequest) {
        case AUDIO_REQ_CUR:
            if (req->bmRequest & AUDIO_REQ_GET_MASK) {
                //GET
                AudioCtl[0] = 0; /* bCUR */
                USBD_CtlSendData (pdev, AudioCtl, req->wLength);
            } else {
                //SET
                /* Prepare the reception of the buffer over EP0 */
                USBD_CtlPrepareRx (pdev, 
                        AudioCtl,
                        req->wLength);
                /* Set the global variables indicating current request and its length 
                to the function usbd_audio_EP0_RxReady() which will process the request */
                AudioCtlCmd = AUDIO_REQ_SET_CUR;     /* Set the request value */
                AudioCtlLen = req->wLength;          /* Set the request data length */
                AudioCtlCS  = bCS;
                AudioCtlUnit = HIBYTE(req->wIndex);  /* Set the request target unit */
            }
            break;

        default:
          USBD_CtlError (pdev, req);
          return;
    }
  }
}

//16.16 UAC2 feedback calc from my nuc505 project
//16.16 value,audio samples per 125us frame(1ms / 8)
// 将十进制数转换为16.16格式
u32 fb1616(float decimal) {

decimal=decimal/8;//for 125us

  // 提取整数部分
  u32 integer_part = (u32)decimal;

  // 提取小数部分
  float fractional_part = decimal - integer_part;

  // 将小数部分转换为16位定点数
  u16 fractional_16_bits = (u16)(fractional_part * 65536); // 2^16

  // 将整数部分和小数部分合并为32位整数
  u32 result = ((u32)integer_part << 16) | (fractional_16_bits & 0xffff);

  return result;
}

void fb(void)
{
uint32_t PlayRate;


switch (working_samplerate)
{
/*
case 384000:

	if (play_speed==0)		PlayRate = (384 << 14) + (0 << 14)/10;
	else if (play_speed>0)	PlayRate = (383 << 14) + (0 << 14)/10;
	else					PlayRate = (385 << 14) + (0 << 14)/10;
	break;
	
case 352800:

	if (play_speed==0)		PlayRate = (352 << 14) + (0 << 14)/10;
	else if (play_speed>0)	PlayRate = (351 << 14) + (0 << 14)/10;
	else					PlayRate = (354 << 14) + (0 << 14)/10;
	break;		
*/	
case 192000:

	if (play_speed==0)		PlayRate = fb1616(192.0);
	else if (play_speed>0)	PlayRate = fb1616(191.0);
	else					PlayRate = fb1616(193.0);
	break;
	
case 176400:

	if (play_speed==0)		PlayRate = fb1616(176.4);
	else if (play_speed>0)	PlayRate = fb1616(177.3);
	else					PlayRate = fb1616(175.4);
	break;
	
case 96000:

	if (play_speed==0)		PlayRate = fb1616(96.0);
	else if (play_speed>0)	PlayRate = fb1616(97.0);
	else					PlayRate = fb1616(95.0);
	break;
	
	
case 88200:

	if (play_speed==0)		PlayRate = fb1616(88.2);
	else if (play_speed>0)	PlayRate = fb1616(89.1);
	else					PlayRate = fb1616(87.2);
	break;
	
case 44100:

	if (play_speed==0)		PlayRate = fb1616(44.1);
	else if (play_speed>0)	PlayRate = fb1616(44.9);
	else					PlayRate = fb1616(43.3);
	break;
	
case 48000:

	if (play_speed==0)		PlayRate = fb1616(48.0);
	else if (play_speed>0)	PlayRate = fb1616(48.9);
	else					PlayRate = fb1616(47.1);
	break;
	
default:
	return;
	
}

//play_speed=0;

	fb_buf[0]=PlayRate & 0xff;
	fb_buf[1]=(PlayRate>>8) & 0xff;
	fb_buf[2]=(PlayRate>>16) & 0xff;
	fb_buf[3]=(PlayRate>>24) & 0xff;	

}


/**
* @brief  usbd_audio_Init
*         Initializes the AUDIO interface.
* @param  pdev: device instance
* @param  cfgidx: Configuration index
* @retval status
*/
static uint8_t  usbd_audio_Init (void  *pdev, uint8_t cfgidx)
{  
				//DCD_EP_Open(pdev, AUDIO_OUT_EP, AUDIO_OUT_PKTSIZE, USB_OTG_EP_ISOC);
				//DCD_EP_Open(pdev, AUDIO_IN_EP, 4, USB_OTG_EP_ISOC);
  return USBD_OK;
}

/**
* @brief  usbd_audio_Init
*         DeInitializes the AUDIO layer.
* @param  pdev: device instance
* @param  cfgidx: Configuration index
* @retval status
*/
static uint8_t  usbd_audio_DeInit (void  *pdev, uint8_t cfgidx)
{ 

  return USBD_OK;
}

/**
  * @brief  usbd_audio_Setup
  *         Handles the Audio control request parsing.
  * @param  pdev: instance
  * @param  req: usb requests
  * @retval status
  */
static uint8_t  usbd_audio_Setup (void  *pdev, 
                                  USB_SETUP_REQ *req)
{
  uint16_t len=USB_AUDIO_DESC_SIZ;
  uint8_t  *pbuf=usbd_audio_CfgDesc + 18;
  
  switch (req->bmRequest & USB_REQ_TYPE_MASK)
  {
    /* AUDIO Class Requests -------------------------------*/
  case USB_REQ_TYPE_CLASS :
    /*
     * bmRequest        bRequest    wValue              wIndex
     * 00100001B        CUR/RANGE    CS                 Entity ID
     * 10100001B                    and CN or MCD       and Interfaces
     *---------------------------------------------------------------------------
     * 00100010B        CUR/RANGE    CS                 Endpoint
     * 10100010B                     and CN or MCD
     *
     */
    switch (req->bmRequest & AUDIO_REQ_CLASS_TYPE_MASK) {
        case AUDIO_REQ_INTERFACE:
            switch (ENTITY_ID(req->wIndex)) {
                case AUDIO_FU_ID:
                    AUDIO_Req_FeatureUnit(pdev, req);
                    break;
                case AUDIO_CLK_ID:
                    AUDIO_Req_ClockSource(pdev, req);
                    break;
            }
            break;
        
        case AUDIO_REQ_ENDPOINT:
            break;
    }
    break;
    
    /* Standard Requests -------------------------------*/
  case USB_REQ_TYPE_STANDARD:
    switch (req->bRequest)
    {
    case USB_REQ_GET_DESCRIPTOR: 
      if( (req->wValue >> 8) == AUDIO_DESCRIPTOR_TYPE)
      {
#ifdef USB_OTG_HS_INTERNAL_DMA_ENABLED
        pbuf = usbd_audio_CfgDesc;   
#else
        pbuf = usbd_audio_CfgDesc + 18;
#endif 
        len = MIN(USB_AUDIO_DESC_SIZ , req->wLength);
      }
      
      USBD_CtlSendData (pdev, 
                        pbuf,
                        len);
      break;
      
    case USB_REQ_GET_INTERFACE :
      USBD_CtlSendData (pdev, (uint8_t *)&alt_setting_now, 1);
      break;
      
    case USB_REQ_SET_INTERFACE :		//switch alt settings
      //if ((uint8_t)(req->wValue) < AUDIO_TOTAL_IF_NUM)
      //{
		switch ( (uint8_t)(req->wValue) )
		{
			case 1 : 	//alt 1 (play)
						alt_setting_now=1;
						EVAL_AUDIO_Stop();
						Play_ptr=0;//reset pointer
						Write_ptr=0;
						DCD_EP_Open(pdev, AUDIO_OUT_EP, AUDIO_OUT_PKTSIZE, USB_OTG_EP_ISOC);
						DCD_EP_Open(pdev, AUDIO_IN_EP, 4, USB_OTG_EP_ISOC);
						DCD_EP_Flush(pdev,AUDIO_IN_EP);
						DCD_EP_PrepareRx(pdev , AUDIO_OUT_EP , (uint8_t*)IsocOutBuff , AUDIO_OUT_PKTSIZE ); 
						//fb();
			
				break;
			case 0 :	//zero bandwidth
						alt_setting_now=0;
						DCD_EP_Close (pdev , AUDIO_OUT_EP);
						DCD_EP_Close (pdev , AUDIO_IN_EP);
						DCD_EP_Flush(pdev,AUDIO_IN_EP);
			
				break;
			default:
			        /* Call the error management function (command will be naked) */
					USBD_CtlError (pdev, req);
				break;
		}	  

     // }
     // else
     // {
        /* Call the error management function (command will be naked) */
      //  USBD_CtlError (pdev, req);
      //}
      break;
    }
  }
  return USBD_OK;
}

static uint8_t  usbd_audio_EP0_TxSent (void  *pdev)
{ 
  return USBD_OK;
}

/**
  * @brief  usbd_audio_EP0_RxReady
  *         Handles audio control requests data.
  * @param  pdev: device instance
  * @retval status
  */
static uint8_t  usbd_audio_EP0_RxReady (void  *pdev)
{ 

  if ( (AudioCtlCmd == AUDIO_REQ_SET_CUR) || (AudioCtlUnit == AUDIO_FU_ID) )    /* request to feture unit */
  {

    /* In this driver, to simplify code, only one unit is managed */
        if (AudioCtlCS == AUDIO_CONTROL_VOLUME) {
            //AUDIO_OUT_fops.VolumeCtl(AudioCtl[0]);
        } else if (AudioCtlCS == AUDIO_CONTROL_MUTE) {
            //AUDIO_OUT_fops.MuteCtl(AudioCtl[0]);
        }
      
      /* Reset the AudioCtlCmd variable to prevent re-entering this function */
      AudioCtlCmd = 0;
      AudioCtlLen = 0;
    
  }

	/* request to endpoint */  //采样频率控制
	else if (AudioCtlReq == AUDIO_REQ_INTERFACE && AudioCtlUnit == AUDIO_CLK_ID && AudioCtlCS  == CS_SAM_FREQ_CONTROL)
	{
		u32 tmpfreq;
		tmpfreq=((AudioCtl[2]<<16)|(AudioCtl[1]<<8)|AudioCtl[0]);
		
		if(working_samplerate==tmpfreq) {printf("s:nc.\r\n");//log
		}
		else{
		
		switch (tmpfreq){		//whitelist avoid bad data
			case 44100:
			case 48000:
			case 88200:
			case 96000:
			case 176400:
			case 192000:
			case 352800:
			case 384000:
				working_samplerate=tmpfreq;
				EVAL_AUDIO_Stop();
				Play_ptr=0;//reset pointer
				Write_ptr=0;
				break;
			default:
				break;
		}
		printf("s:%d.\r\n",tmpfreq);//log
		}
		AudioCtlReq = 0;AudioCtlUnit = 0;AudioCtlCS=0;// Reset the AudioCtlCmd variable to prevent re-entering this function
		
	}  
  
  return USBD_OK;
}

/**
  * @brief  usbd_audio_DataIn
  *         Handles the audio IN data stage.
  * @param  pdev: instance
  * @param  epnum: endpoint number
  * @retval status
  */
static uint8_t  usbd_audio_DataIn (void *pdev, uint8_t epnum)
{
fb_success++;
  return USBD_OK;
}

#if bitdepth==32	
//32bit音频,需要处理一下
/*
u32 switchHL(u32 input)
{
	u32 tmpchrH;
	u32 tmpchrL;
	tmpchrL=(input>>16) & 0xffff;
	tmpchrH =(input& 0xffff) <<16;
	tmpchrH = tmpchrH + tmpchrL;
	return tmpchrH; 
}
*/

u32 switchHL(u32 input)
{
	u32 tmpchr;
	tmpchr=__REV(input);
	return __REV16(tmpchr);
}

#else	
//24bit音频,需要处理一下


u32 switchI24O32(u8 *Ibuffer,u16 offset)
{
	u32 tmpchr;
	u8 Hbyte;
	u8 Mbyte;
	u8 Lbyte;
	Lbyte=Ibuffer[offset+0];
	Mbyte=Ibuffer[offset+1];
	Hbyte=Ibuffer[offset+2];
	tmpchr= (Hbyte<<8) | (Mbyte<<0) |  (Lbyte<<24);
	//tmpchr=tmpchr & 0xff00ffff;
	return tmpchr; 
}
#endif

/**
  * @brief  usbd_audio_DataOut
  *         Handles the Audio Out data stage.
  * @param  pdev: instance
  * @param  epnum: endpoint number
  * @retval status
  */
//输出已完成
u32 rx_bytes_count;
u32 rx_frames_count;
static uint8_t  usbd_audio_DataOut (void *pdev, uint8_t epnum)
{     

u32 i;
vu32 nextbuf;

  if (epnum == AUDIO_OUT_EP)
  { 
	rx_bytes_count	=	USBD_GetRxCount(pdev ,AUDIO_OUT_EP);//实际收到了windows给的多少bytes

	if(rx_bytes_count % bpf != 0)	/// 左右声道数据量不配套？！ 
		{
         //砸了电脑！
		}

    rx_frames_count	=	rx_bytes_count / bpf; // 32bit*2=8byte,,24bit*2=6byte,,16bit*2=4byte

	//PMA到环形缓存拷贝程序
	for (i=0; i<rx_frames_count; i++ )   //CPU屌啊，一个字一个字的拷
	{
	//计算即将写入的位置
		if (Write_ptr < i2s_BUFSIZE ) nextbuf=Write_ptr+2 ;
		else nextbuf=0;
	   
		if (nextbuf != Play_ptr ) //如果没有追尾，把这一帧数据写入到环形缓存
		{
			Write_ptr = nextbuf;
			
		#if bitdepth==32	
			i2s_buf[Write_ptr] = switchHL( IsocOutBuff[i*2] );//32bit left
			i2s_buf[Write_ptr+1] = switchHL( IsocOutBuff[i*2+1] );//32bit right
		#else	
			i2s_buf[Write_ptr]   = switchI24O32( (uint8_t*)IsocOutBuff ,i*6 );//24bit to 32bit left
			i2s_buf[Write_ptr+1] = switchI24O32( (uint8_t*)IsocOutBuff,(i*6+3) );//24bit to 32bit right
		#endif
		}
		else //撞上了
		{
			overrun_counter++;//立OVER-RUN FLAG

			break;//吃吐了，剩下数据全部不要了
		}
	}

	//计算环形数据容量
	if (Write_ptr > Play_ptr) data_remain = Write_ptr - Play_ptr;
	else data_remain = i2s_BUFSIZE - Play_ptr + Write_ptr;

//如果目前是停止状态，有一半容量开启播放
	if ( (audiostatus==0) && (data_remain > i2s_BUFSIZE/2 ) ) EVAL_AUDIO_Play();
//反馈条件	
	if (data_remain > i2s_BUFSIZE/3*2 ) play_speed=-1;//like 44000
	else if (data_remain > i2s_BUFSIZE/2 ) play_speed=0;//like 44100
	else play_speed=1;//like 44200

    /* Toggle the frame index */  
    ((USB_OTG_CORE_HANDLE*)pdev)->dev.out_ep[epnum].even_odd_frame = (((USB_OTG_CORE_HANDLE*)pdev)->dev.out_ep[epnum].even_odd_frame)? 0:1;
      
    //设定OUT端点准备接受下一数据包
    DCD_EP_PrepareRx(pdev,AUDIO_OUT_EP,(uint8_t*)IsocOutBuff,AUDIO_OUT_PKTSIZE);   

	fb();	//update fb value
	DCD_EP_Tx(pdev, AUDIO_IN_EP, fb_buf, 4);	
	
  }
  return USBD_OK;
}

/**
  * @brief  usbd_audio_SOF
  *         Handles the SOF event (data buffer update and synchronization).
  * @param  pdev: instance
  * @param  epnum: endpoint number
  * @retval status
  */
static uint8_t  usbd_audio_SOF (void *pdev)
{     
	if(alt_setting_now)
	{
	//DCD_EP_Flush(pdev,AUDIO_IN_EP);
	//DCD_EP_Tx(pdev, AUDIO_IN_EP, fb_buf, 4);
	//DCD_EP_PrepareRx(pdev , AUDIO_OUT_EP , (uint8_t*)IsocOutBuff , AUDIO_OUT_PKTSIZE ); 
	}
  
  return USBD_OK;
}
/**
  * @brief  usbd_audio_IN_Incplt
  *         Handles the iso out incomplete event.
  * @param  pdev: instance
  * @retval status
  */
static uint8_t  usbd_audio_IN_Incplt (void  *pdev)
{
fb_incomplt++;
  return USBD_OK;
}

/**
  * @brief  usbd_audio_OUT_Incplt
  *         Handles the iso out incomplete event.
  * @param  pdev: instance
  * @retval status
  */
static uint8_t  usbd_audio_OUT_Incplt (void  *pdev)
{
    /* Toggle the frame index */  
    ((USB_OTG_CORE_HANDLE*)pdev)->dev.out_ep[AUDIO_OUT_EP].even_odd_frame = (((USB_OTG_CORE_HANDLE*)pdev)->dev.out_ep[AUDIO_OUT_EP].even_odd_frame)? 0:1;
      
    //设定OUT端点准备接受下一数据包
    DCD_EP_PrepareRx(pdev,AUDIO_OUT_EP,(uint8_t*)IsocOutBuff,AUDIO_OUT_PKTSIZE);   


rx_incomplt++;
  return USBD_OK;
}


/**
  * @brief  USBD_audio_GetCfgDesc 
  *         Returns configuration descriptor.
  * @param  speed : current device speed
  * @param  length : pointer data length
  * @retval pointer to descriptor buffer
  */
static uint8_t  *USBD_audio_GetCfgDesc (uint8_t speed, uint16_t *length)
{
  *length = sizeof (usbd_audio_CfgDesc);
  return usbd_audio_CfgDesc;
}


/* AUDIO interface class callbacks structure */
USBD_Class_cb_TypeDef  CLASS_cb = 
{
  usbd_audio_Init,
  usbd_audio_DeInit,
  usbd_audio_Setup,
  usbd_audio_EP0_TxSent, /* EP0_TxSent */
  usbd_audio_EP0_RxReady,
  usbd_audio_DataIn,
  usbd_audio_DataOut,
  usbd_audio_SOF,
  usbd_audio_IN_Incplt,
  usbd_audio_OUT_Incplt,   
  USBD_audio_GetCfgDesc,
#ifdef USB_OTG_HS_CORE  
  USBD_audio_GetCfgDesc, /* other speed desc */
#endif    
};

/**
  * @}
  */ 

/**
  * @}
  */ 

/**
  * @}
  */ 

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
