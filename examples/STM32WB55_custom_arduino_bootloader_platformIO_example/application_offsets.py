## this script is used to help indicate when the bootloader is too large.
## NOTE: this script does not check the size itself, but rather updates the believed max flash size, which platformIO shows after compiling

Import("env")
# print("bootloader_size_check.py has ran") # basic debug
config = env.GetProjectConfig() # access the overall project
PROGRAM_START_str : str = config.get("TOTALB", "PROGRAM_START") # fetch variable from other env
PROGRAM_START = int(PROGRAM_START_str, 16 if (PROGRAM_START_str.startswith("0x")) else 10) # convert to int
# print("PROGRAM_START:", PROGRAM_START_str, PROGRAM_START)
flash_base : int = 0x08000000 # TODO: retrieve automatically (instead of hardcoding like this)
# print("flash_base + PROGRAM_START:", hex(flash_base + PROGRAM_START), type(flash_base))
board_config = env.BoardConfig() # fetch board configuration of current env
board_config.update("upload.offset_address", hex(flash_base + PROGRAM_START)) # indicate where to start uploading the new program
board_config.update("build.flash_offset", PROGRAM_START) # tell platformio-build.py to tell the compiler & linker to offset flash.
## NOTE: from STM32duino version 2.7.0 onwards, platformio-build will (finally) set LD_FLASH_OFFSET flag and define VECT_TAB_OFFSET based on build.flash_offset