/*
this code demonstrates a basic bootloader and app.
These 2 codes are currently in the same file, seperated by an ifdef, but this is not necessary, and just comes down to preference
(presumably, the average sane coder might prefer to have 2 seperate main files for the bootloader and app codes (or even 2 seperate PIO projects))

Note: i wrote this code to implement my own OTA (BLE) bootloader, so when you see OTA mentioned just think <your-bootloader-purposes-here>
(to be specific, i need an un-brickable OTA bootloader for a device i can no longer directly access after deployment)

To verify whether uploading is an issue, use STM32CubeProgrammer to look at the flash
You should see the bootloader start at 0x8000000 and end before PROGRAM_START (currently 0x8007000),
 and the user app start at PROGRAM_START (currently 0x8007000).
If you see only 1 program, with the other being erased every time, try to mess with the uploader settings to avoid erasing certain sections.

The code below is still in its infancy in terms of security,
 by which i mean the fact that it will happily jump to the user program, even if it's all 0's
*/

#include <Arduino.h>


#ifdef TOTALB_PROGRAM_START // a quick'n'dirty compiler check to see if this is the bootloader code, or the user-application code
  #include "TOTALB_FUNCS.h"

  void setup() {
    /* enabled watchdogs (and similar anit-crash/anit-brick assurances) first */

    // Serial.setRx(debug_RX);  Serial.setTx(debug_TX); // alternate pins
    Serial.begin(115200);  /* delay(10); */ Serial.print("\nboot(loader) "); Serial.print(millis()); Serial.print("\t"); Serial.println(micros());

    #ifdef TOTALB_PROGRAM_START
      Serial.print("TOTALB_PROGRAM_START: 0x"); Serial.println(TOTALB_PROGRAM_START, HEX);
    #endif
    Serial.print("SCB->VTOR: 0x"); Serial.println(SCB->VTOR, HEX);
    #ifdef VECT_TAB_OFFSET // the bootloader itself should not have an offset (but i guess there is not a whole lot preventing you)
      Serial.print("VECT_TAB_OFFSET(?!): 0x"); Serial.println(VECT_TAB_OFFSET, HEX);
    #endif

    bool jump = true; // the bootloader has to decide whether or not to jump to the user-application

    //// first, a little sanity check to make sure TOTALB_PROGRAM_START actually makes sense
    jump &= TOTALB::checkJumpLocation(); // checks whether jumping to TOTALB_PROGRAM_START would be entirely crazy

    /* determine 'jump' boolean based on persistant registers/flash, watchdogs, reset causes, etc. */

    if(jump) { // just move on to user-program
      delay(25); // allow some time for the serial output buffer to clear
      TOTALB::jumpToProgram(); // un-initializes microcontroller and jumps to user-application ResetHandler()
      //// this point is only reached if jumpToProgram() refused to jump (if, for example, checkJumpLocation() returned false)
      //// it's important to note that the clocks have been disabled and peripherals reset at this point,
      ////  so your best course of action is make sure next time around jump==false (e.g. by setting a flag in flash/backup-domain and calling a software-reset)
      while(1); // in this example the program just stops. (again, serial debug would require re-initialization of clocks & peripherals)
    }
    //// else:
    //// do bootloader stuff! (in my case, that's BLE OTA)
  }

  void loop() { // the bootloader's loop only runs if it didn't jumpToProgram()
    Serial.print("loop BootLoader "); Serial.print(millis()); Serial.print("\t"); Serial.println(micros());
    delay(1000);
  }

#else // TOTALB_PROGRAM_START (everything below is the user-application code (post-bootloader)
//////////////////////////////////////////////////////////////////////////////////////////// application code //////////////////////////////////////////////////////////
  
  /*
    The user app is just normal code, really nothing special about it,
    EXCEPT, you might want to think of some clever app <-> bootloader communication.
    For example, how do you plan to enter the bootloader?
      with OTA flashing, it's usually through a special (in my case BLE) command.
    You should probably also seriously think about using a watchdog,
      and checking in the bootloader code what the reset cause was.
    In my case, i won't have access to the device once deployed,
      so an un-brick-able bootloader is absolutely crucial.
  */

  void setup() {
    /* consider updating watchdog timeout (but i can't recommend disabling them entirely) */
    // Serial.setRx(debug_RX);  Serial.setTx(debug_TX); // alternate pins
    Serial.begin(115200);  /* delay(10); */ Serial.print("\nboot app "); Serial.print(millis()); Serial.print("\t"); Serial.println(micros());

    #ifdef TOTALB_PROGRAM_START // regular apps should not have this value defined
      Serial.print("TOTALB_PROGRAM_START(?!): "); Serial.println(TOTALB_PROGRAM_START, HEX);
    #endif
    Serial.print("SCB->VTOR: "); Serial.println(SCB->VTOR, HEX);
    #ifdef VECT_TAB_OFFSET // apps (which go after the bootloader) should have this value defined
      Serial.print("VECT_TAB_OFFSET: "); Serial.println(VECT_TAB_OFFSET, HEX);
    #endif
  }

  void loop() {
    /* make sure to handle watchdogs, if enabled */
    Serial.print("loop AP "); Serial.print(millis()); Serial.print("\t"); Serial.println(micros());
    delay(500);
  }

  //// possible code for bootloader<->app communication (found in 'libraries\SrcWrapper\src\stm32\bootloader.c')
  // enableBackupDomain();
  // //// official (existing) HID bootloader (v2.2+) use the following space in the backup-registers:
  // setBackupRegister(HID_MAGIC_NUMBER_BKP_INDEX, HID_MAGIC_NUMBER_BKP_VALUE);
  // #ifdef HID_OLD_MAGIC_NUMBER_BKP_INDEX
  //   /* Compatibility to the old HID Bootloader (ver <= 2.1) */
  //   setBackupRegister(HID_OLD_MAGIC_NUMBER_BKP_INDEX, HID_MAGIC_NUMBER_BKP_VALUE);
  // #endif
  // /* NOTE: the backup register available are: LL_RTC_BKP_DR0  up to  LL_RTC_BKP_DR19  which each hold a 32bit value.
  // NVIC_SystemReset(); // software reset

#endif // TOTALB_PROGRAM_START (end of regular app code)