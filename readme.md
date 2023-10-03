
# Cheapest STM32F2 Asynchronous USB Audio2  
### Using stm32f205+usb3300,Support external audio master clock   
   
![Image pcb](https://github.com/rushxrushx/stm32f4_usbaudio2/blob/master/image/testpcb.jpg?raw=true)  
  
compiler: KEIL MDK-ARM 4.74.0.22   
note: windows PC need uac2 asio driver  
  
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
  