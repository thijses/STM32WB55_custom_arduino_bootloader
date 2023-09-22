## this script is used to help indicate when the bootloader is too large.
## NOTE: this script does not check the size itself, but rather updates the believed max flash size, which platformIO shows after compiling

Import("env")
# print("bootloader_size_check.py has ran") # basic debug
config = env.GetProjectConfig() # access the overall project
maxSize : str = config.get("TOTALB", "PROGRAM_START") # fetch variable from other env
# print("PROGRAM_START:", maxSize, int(maxSize, 16 if (maxSize.startswith("0x")) else 10))
maxSize : int = int(maxSize, 16 if (maxSize.startswith("0x")) else 10) # convert str to int
try:
    comm_area = config.get("TOTALB", "BOOTLOADER_COMM_SIZE")
    # print("BOOTLOADER_COMM_SIZE:", comm_area, int(comm_area, 16 if (comm_area.startswith("0x")) else 10))
    comm_area = int(comm_area, 16 if (comm_area.startswith("0x")) else 10) # convert str to int
    maxSize -= comm_area
except:
    # print("BOOTLOADER_COMM_SIZE not found") # just to let the user know (not actually an issue)
    doNothing = 0
board_config = env.BoardConfig() # fetch board configuration of current env
board_config.update("upload.maximum_size", maxSize) # update max size (to show bootloader size limit)