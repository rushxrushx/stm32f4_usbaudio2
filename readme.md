
# Cheapest STM32 Asynchronous USB Audio2  
### with external audio master clock   
   
![Image pcb](https://github.com/rushxrushx/stm32f4_usbaudio2/blob/master/image/testpcb.jpg?raw=true)  
  
This is uac2 project for stm32 series F205/F405/F207/F407 with builtin OTG-HS can be use.  
OTG-HS not come with internal HS PHY, you need USB3300 ULPI PHY chip.  
note: This project not working with MS audio driver,Windows need UAC2 driver,eg:Xmos usb stereo driver   
stereo driver support up to 32/192k,if you want 32/384,try JLaudio driver(device PID VID must charged).
and clock input must increse to 90MHZ/98MHZ because stm32 i2s minium is 256fs ...I dont tried.  
I just tried 32/384 data rate is okay.USB Data not lost but i2s with 45/49MHZ clock only run 260khz  max...
  

compiler and IDE: KEIL MDK-ARM 4.74.0.22   
  

    
### v20250226
 - add: mini 9018 hardware  
 - fix: missing usb3300 reset
 - fix: explicit feedback total wrong,it is 16.16 format sample count for per 125us.   
    
### v20240221  
 - fix: dma performance on 192k
 - led color minor change   
   
### v20231112  
 - add: probably missing dma reset before play  
  
### v20231008  
 - add: cmbacktrace  
 - and fixed :hardfault :IsoInincomptete jmp to null    
 - clean: uac1 uac2 code  
  
### v0.1   
20231002  
inital buggy release  
  
### v0.2  
20231003  
fix: outEp descripter no async  
fix: async ep1 data TX to PC,fix missing fb,now fb working.  
fix: sample frequency setup(data receive must in EP0Rxready)  
add: reset i2s engine when enter playing altset  
fix: some var must add 'Voliate'  
add: UART log ring buffer system,prevent 'while' when print logs.  
  

  
thanks to:  
uac2 code from https://github.com/coflery/STM32F4_USB_SoundCard  
ALITENTEK devboard demo code (and ST demo code)  

PCB thanksto:
stm32f405+usb3300 DAPlink sch and PCB
