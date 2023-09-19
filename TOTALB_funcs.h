/*
hardly a library, just some functions useful for making custom bootloaders



ideas to incorporate:
- watchdog!!!! (see IWatchdog library?)
- stuff to keep in 'user memory' block(?):
  + rebootRequest_to_OTA boolean (obviously)
  + CRC/program_size/magic_word_check/reset_cause
  + bootloader update options (bool or password)
  + watchdog-like settings & values:
    * the app could set a flag high when it has been running for 10+ seconds. If the bootloader finds this flag low x times in-a-row (in combination with POR flags?)
          it could indicate that the device is being rapidly power-cycled (like toggling a light switch of a smart-lamp several times to put it in reset mode).
          (note: should be disable-able using a seperate flag, in case a different (foolproof/assured) method of entering the OTA bootloader is preferred)
    * (if the Qi WLP decides to end power-transfer of its own volition, maybe indicate to the bootloader that this is the case???)
- flash write-access locks for bootloader section?
  + activated in bootloader, disabled by reset?
  + or password-protected (if hardware has that capability)
  + or disabled using the shared memory block (communiation between main code and bootloader between resets) bool/password
  + or disabled using special HW input (IR,NFC,etc.)
  + see HAL_FLASHEx_OBProgram() (with comments from source file)

it should be totally absolutely un-brick-able,
so things to avoid include:
- write-access to bootloader code(?)
- being unable to enter the bootloader (if the user app doesn't crash, but fails to activate the bootloader, it's functionally bricked)

reset cause flags?:
LL_RCC_IsActiveFlag_IWDGRST()  Independent Watchdog
LL_RCC_IsActiveFlag_LPWRRST()  Low Power
LL_RCC_IsActiveFlag_OBLRST()   Option Byte Loading (required when changing option bytes (see 'OB' tab in STM32CubeProgrammer))
LL_RCC_IsActiveFlag_PINRST()   Pin
LL_RCC_IsActiveFlag_SFTRST()   Software
LL_RCC_IsActiveFlag_WWDGRST()  Window Watchdog
LL_RCC_IsActiveFlag_BORRST()   Brown Out
LL_RCC_ClearResetFlags(); clears reset flags (Note: try to find, see if weak reset handler already exists)
(IWatchdog library might also have some of these, not sure)


*/
#ifndef TOTALB_FUNCS_h // include only once
#define TOTALB_FUNCS_h

// #ifndef TOTALB_PROGRAM_START
//   #error("TOTALB_funcs.h error: TOTALB_PROGRAM_START not defined! Only include this header if this is bootloader code")
// #endif

#include <Arduino.h> // used for Serial (for debugging)

#include <stm32wbxx_hal_flash_ex.h> // for HAL_FLASHEx_OBGetConfig

namespace TOTALB {

//// there is some variant-specific code below:
#ifdef ARDUINO_P_NUCLEO_WB55RG  // Note: alternative defines: STM32WB55xx
  #include <lock_resource.h> // for undo_SystemClock_Config() 
  // #include <shci.h> // from STM32duinoBLE library, for SHCI_C2_Reinit() (to reset CPU2 (BLE) before jump). Commented out because i don't want to include STM32duinoBLE
  
  /// @brief undoes what SystemClock_Config() did (see variant_P_NUCLEO_WB55RG.cpp) to bring clocks to a state similar to just-after-reset
  /// @param leave_LSE_same leaves LSE as it is (NOTE: might cause niche issues, BUT it does save ~124ms of re-initialization time after jump)
  /// @return HAL_OK, unless something is very wrong
  HAL_StatusTypeDef undo_SystemClock_Config(bool leave_LSE_same=false) {
    /** revert the clocks & oscilators to their reset values
    the order of these steps is very important!
    The SystemClock_Config function does it forwards, like:
      - enable oscillators
      - enable main clock and dividers
      - enable peripheral clocks
      - enable SMPS
    so this function should basically go in reverse
      - disable SMPS (not sure this is actually needed, but whatever)
      - disable/default peripheral clocks (Note: full peripheral resets are done later, using HAL_DeInit())
      - switch main clock back to default (Multi Speed Internal @ 4MHz)
      - disable oscillators
    The use of HAL functions is strongly preferred, as they account for a lot of edge-cases and eventualities already
    */
    RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {};
    RCC_OscInitTypeDef RCC_OscInitStruct = {};
    /* This prevents concurrent access to RCC registers by CPU2 (M0+) */
    hsem_lock(CFG_HW_RCC_SEMID, HSEM_LOCK_DEFAULT_RETRY);
    /* Select HSI as system clock source after Wake Up from Stop mode */
    LL_RCC_SetClkAfterWakeFromStop(LL_RCC_STOP_WAKEUPCLOCK_MSI); // MSI is default
    LL_PWR_SMPS_Disable(); // disable instead of enable
    LL_PWR_SMPS_SetOutputVoltageLevel(LL_PWR_SMPS_OUTPUT_VOLTAGE_1V50); // note: this function uses factory calib data to reach target
    LL_PWR_SMPS_SetStartupCurrent(LL_PWR_SMPS_STARTUP_CURRENT_220MA); // default is 111 -> (80+7*20 = 220mA)
    /* Initializes the peripherals clocks */
    /* RNG needs to be configured like in M0 core, i.e. with HSI48 */
    PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_SMPS | RCC_PERIPHCLK_RFWAKEUP
                                              | RCC_PERIPHCLK_RNG | RCC_PERIPHCLK_USB;
    PeriphClkInitStruct.UsbClockSelection = RCC_USBCLKSOURCE_HSI48; // already at default (i think?)
    PeriphClkInitStruct.RngClockSelection = RCC_RNGCLKSOURCE_CLK48; // CLK48 is default (i think)
    PeriphClkInitStruct.RFWakeUpClockSelection = RCC_RFWKPCLKSOURCE_NONE; // NONE is default
    PeriphClkInitStruct.SmpsClockSelection = RCC_SMPSCLKSOURCE_HSI; // HSI is default
    PeriphClkInitStruct.SmpsDivSelection = RCC_SMPSCLKDIV_RANGE0; // range 0 is default
    HAL_StatusTypeDef err = HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct);
    if (err != HAL_OK) { /* Error_Handler(); */ return(err); }
    /* Configure the SYSCLKSource, HCLK, PCLK1 and PCLK2 clocks dividers */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK4 | RCC_CLOCKTYPE_HCLK2
                                  | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                  | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_MSI; // MSI instead of HSE
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1; // already at default
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1; // already at default
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1; // already at default
    RCC_ClkInitStruct.AHBCLK2Divider = RCC_SYSCLK_DIV1; // already at default
    RCC_ClkInitStruct.AHBCLK4Divider = RCC_SYSCLK_DIV1; // already at default
    err = HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0); // set to default flash latency 0 (and because MSI@4MHz < HSE@32MHz)
    if (err != HAL_OK) { /* Error_Handler(); */ return(err); }
    /* Initializes the CPU, AHB and APB busses clocks */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI | RCC_OSCILLATORTYPE_HSI48
                                      | RCC_OSCILLATORTYPE_HSE | ((!leave_LSE_same) ? RCC_OSCILLATORTYPE_LSE : 0);
    RCC_OscInitStruct.HSEState = RCC_HSE_OFF; // turn off
    RCC_OscInitStruct.HSIState = RCC_HSI_OFF; // turn off
    RCC_OscInitStruct.HSI48State = RCC_HSI48_OFF; // turn off
    RCC_OscInitStruct.LSEState = RCC_LSE_OFF; // turn off (unless leave_LSE_same is used (to save ~124ms re-init time))
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT; // already at default
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE; // already at default
    err = HAL_RCC_OscConfig(&RCC_OscInitStruct);
    if (err != HAL_OK) { /* Error_Handler(); */ return(err); }
    hsem_unlock(CFG_HW_RCC_SEMID); // clock alterations are done, let go of semaphore
    return(HAL_OK); // basically a formality, as i don't know what to do if this actually fails
  }

  /// @brief get the SFSA from the option bytes. It's good to know where CPU2's restricted territory begins
  /// @return the memory address (including FLASH_BASE) where the BLE stack starts (don't overwrite from this point on)
  uint32_t getSecureFlashStartAddr() {
    //// see HAL_FLASHEx_OBGetConfig() comments
    FLASH_OBProgramInitTypeDef pOBInit; // struct to store output in
    HAL_FLASHEx_OBGetConfig(&pOBInit); // get all option bytes (because I can't call FLASH_OB_GetSecureMemoryConfig() directly)
    return(pOBInit.SecureFlashStartAddr); // e.g. 0x080E0000 for the V1.16.0 HCILayer_fw
  }
#else // the undo_SystemClock_Config() is (currently) specific to the WB55RG. If a different board is selected, it may result in silent hardcrashes.
  #error("alternate board selected, check undo_SystemClock_Config() in variant_.cpp and use reference manual to make undo_SystemClock_Config()")
#endif

/// @brief check whether jumping to TOTALB_PROGRAM_START is (likely to be) safe
/// @return true if it looks safe enough, false if you should ABSOLUTELY NOT jump there
template<uint32_t __PROGRAM_START>
bool _checkJumpLocation() {
  //// first and foremose, check if the flash 
  const uint32_t* userAppStartPtr = (uint32_t*)(FLASH_BASE + __PROGRAM_START); // (a pointer to) the place in flash where the actual app starts
  const uint32_t resetHandlerAddress = *(userAppStartPtr + 1); // find the reset handler function (2nd 32bit value) in the app's vector table (Note +1=+4_bytes=+(sizeof(ptr)))
  uint32_t newStackAddress = *userAppStartPtr; // read new stack pointer from flash (1st value of program) (usually it's 0x20030000)
  if(((newStackAddress == 0xFFFFFFFF) || (newStackAddress == 0x00000000))
      || ((resetHandlerAddress == 0xFFFFFFFF) || (resetHandlerAddress == 0x00000000))) { return(false); } // the stack pointer should not look like empty flash
  //// similarly, you could check if you're about to jump into restricted space (like CPU2's Secure Flash)
  uint32_t SFSA = getSecureFlashStartAddr(); // obtain SFSA from option bytes (this is where CPU2's restricted territory begins)
  if((FLASH_BASE + __PROGRAM_START) >= SFSA) { return(false); } // absolutely DO NOT jump to areas that CPU1 has no business in
  //// else
  return(true);
}

#ifdef TOTALB_PROGRAM_START
  /// @brief check whether jumping to TOTALB_PROGRAM_START is (likely to be) safe
  /// @return true if it looks safe enough, false if you should ABSOLUTELY NOT jump there
  bool checkJumpLocation() { return(_checkJumpLocation<TOTALB_PROGRAM_START>()); }
#endif

/// @brief (semi-private template version (not recommended)) de-initialize clocks & peripherals and attempt to start program at <__PROGRAM_START>
/// @param leave_LSE_same leaves LSE as it is (NOTE: might cause niche issues, BUT it does save ~124ms of re-initialization time after jump)
template<uint32_t __PROGRAM_START>
void _jumpTo(bool leave_LSE_same=false) {
  //// before jumping to the new app's reset handler, the system needs to be brought to reset-like state
  /* NOTE: call SHCI_C2_Reinit() from the STM32duinoBLE library (or Cube FW?) for resetting CPU2 (in case BLE was used before jump)*/
  //// the most important factor is resetting the clock & oscillator configs (without this step, it would crash so hard that it takes JTAG debuggers down with it)
  if(undo_SystemClock_Config(leave_LSE_same) != HAL_OK) { return; } // should always return HAL_OK
  //// repeated initialization of peripherals like the USART will also result in a nice silent crash..., so we're resetting all the peripherals next:
  HAL_DeInit(); // resets all peripherals (always returns HAL_OK)
  //// alternatively, you could overload the weak function HAL_MspDeInit() and make it call undo_SystemClock_Config(). HAL_DeInit() will call HAL_MspDeInit() after reset peripherals
  
  /* other stuff to potentially turn off:
    - the Clock Security System for LSE can be turned off, but the HSE CSS can only be turned off with full reset. Should be fine to ignore though
      + if(LL_RCC_IsEnabledIT_LSECSS) { LL_RCC_DisableIT_LSECSS(); } // disable LSE Clock Security System interrupt (if enabled)
    - resetting the RTC (and backup domain stuff)
      + PWR_CR1 must be set first??? (to gain write-access to RCC_BDCR)
      + LL_RCC_ForceBackupDomainReset(); LL_RCC_ReleaseBackupDomainReset(); // backup domain software reset
    - stuff from hw_config_init()
      + USB should probably be disabled in some way, but it was already reset in HAL_DeInit(). Not sure if that also disables it!
      + CRC can be ignored i think
      + DWT is for debugging (JTAG?), so it can be left on
      + IPclock can be ignored (left enabled?), i think(?)
  */
  const uint32_t* userAppStartPtr = (uint32_t*)(FLASH_BASE + __PROGRAM_START); // (a pointer to) the place in flash where the actual app starts
  uint32_t newStackAddress = *userAppStartPtr; // read new stack pointer from flash (1st value of program) (usually it's 0x20030000)
  typedef void (*fct_t)(void);
  fct_t app_reset_handler = (fct_t) *(userAppStartPtr + 1); // find the reset handler function (2nd 32bit value) in the app's vector table (Note +1=+4_bytes=+(sizeof(ptr)))
  //// you can do some last-minute reassuring checks here, like: (Note: the clocks and peripherals have been disabled though, so debug printing is not easy)
  // if(!checkJumpLocation()) { return; } // the stack pointer should not look like empty flash
  //// now jump to the app:
  SCB->VTOR = (volatile uint32_t) userAppStartPtr; // set the Vector table pointer to the app's one
  __set_MSP(newStackAddress); // switch stack address pointer to the app's one (1st 32bit value in the vector table) (which is the stack pointer address) (probably the same as current)
  app_reset_handler(); // start the app, by running its reset handler. From this point on, the app should be running
  /* app_reset_handler() never returns.
  * However, if for any reason a PUSH instruction is added at the entry of  JumpFwApp(),
  * we need to make sure the POP instruction is not there before app_reset_handler() is called
  * The way to ensure this is to add a dummy code after app_reset_handler() is called
  * This prevents app_reset_handler() to be the last code in the function.
  */
  __WFI(); // 'wait for interrupt'
}

#ifdef TOTALB_PROGRAM_START
  /// @brief de-initialize clocks & peripherals and attempt to start user-application (@ TOTALB_PROGRAM_START)
  /// @param leave_LSE_same leaves LSE as it is (NOTE: might cause niche issues, BUT it does save ~124ms of re-initialization time after jump)
  void jumpToProgram(bool leave_LSE_same=false) { _jumpTo<TOTALB_PROGRAM_START>(leave_LSE_same); }
#endif

/// @brief check whether jumping to the bootloader is (likely to be) safe (it ABSOLUTELY SHOULD BE, BTW)
/// @return true if it looks safe enough, false if you should ABSOLUTELY NOT jump there
bool checkBootloaderLocation() { return(_checkJumpLocation< 0 >()); }
/// @brief (not recommended, software-reset is preferred) de-initialize clocks & peripherals and attempt to start bootloader (again)
/// @param leave_LSE_same leaves LSE as it is (NOTE: might cause niche issues, BUT it does save ~124ms of re-initialization time after jump)
void jumpToBootloader(bool leave_LSE_same=false) { _jumpTo< 0 >(leave_LSE_same); } // bootloader starts at FLASH_BASE, so offset == 0

} // namespace TOTALB

#endif // TOTALB_FUNCS_h