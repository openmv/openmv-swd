# openmv-swd
Serial Wire Debugger Programmer

The Serial Wire Debugger (SWD) Programmer can load firmware on up to five STM32F427/STM32F765 microcontrollers at the same time. The SWD Programmer is powered by the Propeller Chip. All eight cores of the Propeller Chip are used by the SWD Programmer firmware. Each SWD programming thread is run in a separate core. A master core controls the five SWD programming cores. Finally, one core provides SD card services while another core handles button debouncing and LED PWM.

If you're interested in practical code demonstrating how to load the firmware on an ST32F427/STM32F765 microcontroller, or generally, any ST Cortex-M4/Cortex-M7 device checkout the [SWD Code](http://github.com/openmv/openmv-swd/blob/master/module/V2/src/SWD.spin).

# To build the OpenMV SWD application:

* Install 7-Zip and add it to PATH.
* Install Python 2.7 and add it to PATH.
* Install Qt (to the default location).
* Install The Qt Installer Framework (to the default location).

In `/`, build the application (using the standard bare terminal):

     make.py

You'll find the installer in `module/build`.
