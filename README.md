# STM32WB55_custom_arduino_bootloader
an example of custom bootloader implementation for the STM32WB55 which uses regular arduino-platform code for the bootloader.

I've named it (somewhat egocentrically): Thijs Over-The-Air Loader-of-Boots (because my name is Thijs and i'm using this to make a BLE OTA bootloader)

There exist bootloaders for STM32 devices, and ST's examples sketches includes an OTA bootloader for the STM32WB55 specifically, HOWEVER, most of these examples write the bootloader in lower-level C, and alter source files to insert the bootloader early in the startup process. This platformIO project however, starts up like a normal STM32-arduino-platform device, which lets you write the bootloader in the trusty setup() & loop() functions we've all grown accustomed to. Existing bootloaders are often inserted early in the startup process because (it's faster, but also because) it means you can run the regular initialization functions afterwards. This code differs, in that it initializes to full arduino-levels (of comfort/ease), and then (in order to jump to the user-application) un-initializes everything again, just so it can be safely re-initialized by the user-application. I spent several days pouring over the reference manual and the various layers of STM32 abstraction to find the default register settings and the correct order to deactivate things. (it mostly took a while because the STM32WB55 will crash so hard that even the ST-link debugger becomes unresponsive).

Jumping to the user program takes about ~1.5ms if using 'leave_LSE_same'  (asin "jumpToProgram(leave_LSE_same = true)"). RE-initializing the LSE after it's been turned off takes about ~124ms (with both the nucleo dev kit and my custom boards). The whole RTC peripheral is reset, so leaving the LSE crystal running should be fine, but there might be some niche problematic scenarios (e.g. involving the LSE Clock Security System).

Usage: (see example project)
- (to compile the example script, copy the whole library into the lib folder of that project)
- in the platformio.ini file, there are 2 targets. One is for uploading bootloader code and the other is for uploading application code.
- Upload the user-app (by selecting the 'OTA_z_STM32WB55' target) (Note: if your uploader offset settings are correct, this should not overwrite bootloaders)
- then upload the bootloader (by selecting the 'BOOTL_STM32WB55' target)
- watch the serial monitor show it booting twice


current problems i'm trying to solve:
- find a way to perform SHCI_C2_Reinit() (from the STM32duinoBLE library, but without importing the whole library), or perhaps a more universal CPU2 reset
- add some basic watchdog example code
- add some basic flash/backup-domain flag example code
