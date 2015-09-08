# openmv-swd
Serial Wire Debugger Programmer

The Serial Wire Debugger (SWD) Programmer can load firmware on up to five STM32F427 microcontrollers at the same time. The SWD Programmer is powered by the Propeller Chip. All eight cores of the Propeller Chip are used by the SWD Programmer firmware. Each SWD programming thread is run in a seperate core. A master core controls the five SWD programming cores. Finally, one core provides SD card services while another core handles button deboucning and LED PWM.

If you're intrested in practical code demonstarting how to load the firmware on an ST32F427 microcntroller, or generally, any ST Cortext-M4 device checkout the [SWD Code](https://github.com/opnmv/openmv-swd/blob/master/src/SWD.spin).
