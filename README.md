# STM32WB55_custom_arduino_bootloader
an example of custom bootloader for the STM32WB55 which uses regular arduino-platform code for the bootloader.

There exist bootloaders for STM32 devices, and ST's examples sketches includes an OTA bootloader for the STM32WB55 specifically, HOWEVER, most of these examples write the bootloader in lower-level C, and alter source files to insert the bootloader early in the startup process. This platformIO project however, starts up like a normal STM32-arduino-platform device, which lets you write the bootloader in the trusty setup() & loop() functions we've all grown accustomed to. Existing bootloaders are often inserted early in the startup process because (it's faster, but also because) it means you can run the regular initialization functions afterwards. This code differs, in that it initializes to full arduino-levels (of comfort/ease), and then (in order to jump to the user-application) un-initializes everything again, just so it can be safely re-initialized by the user-application. I spent several days pouring over the reference manual and the various layers of STM32 abstraction to find the default register settings and the correct order to deactivate things. (it mostly took a while because the STM32WB55 will crash so hard that even the ST-link debugger becomes unresponsive). Unfortunately, it does (currently) take about 124ms between jumping to the user-application and its setup() function running. I am still working on identifying this delay (it might be oscillator-related, not sure yet)

Usage: (see example project)
- replace the platformio-build.py script in .platformio\packages\framework-arduinoststm32\tools\platformio  with the one in patches\    (this makes setting a VECT_TAB_OFFSET possible)
- in the platformio.ini file, there are 2 targets. One is for uploading bootloader code and the other is for uploading application code. (Note: currently, my ST-links are overwriting the bootloader when uploading user-applications. Very annoying, will fix soon).
- Upload the user-app (by selecting the 'OTA_z_STM32WB55' target)
- then upload the bootloader (by selecting the 'BOOTL_STM32WB55' target)
- watch the serial monitor show it booting twice


current problems i'm trying to solve:
- patching the platformio-build.py script automatically (and perhaps more eligantly than just replacing it)
- find why it takes 124 milliseconds to boot after jumping
- test bluetooth before and after jump
- add some basic watchdog suggestions
- add some basic flash/backup-domain flag suggestions
- (work on the full BLE OTA code that relies on this stuff)
