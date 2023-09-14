## this script is used to help indicate when the bootloader is too large.
## NOTE: this script does not check the size itself, but rather updates the believed max flash size, which platformIO shows after compiling

Import("env")
# print("bootloader_size_check.py has ran") # basic debug
config = env.GetProjectConfig() # access the overall project
PROGRAM_START_str : str = config.get("TOTALB", "PROGRAM_START") # fetch variable from other env
# print("PROGRAM_START:", PROGRAM_START_str, int(PROGRAM_START_str, 16 if (PROGRAM_START_str.startswith("0x")) else 10))
board_config = env.BoardConfig() # fetch board configuration of current env
board_config.update("upload.offset_address", int(PROGRAM_START_str, 16 if (PROGRAM_START_str.startswith("0x")) else 10)) # indicate where to start uploading the new program