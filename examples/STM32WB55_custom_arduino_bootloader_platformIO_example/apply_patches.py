## this script replaces the platformio-build.py file with one that allows program offsets


# from os.path import join, isfile

# Import("env")

# FRAMEWORK_DIR = env.PioPlatform().get_package_dir("framework-arduinoststm32")
# patchflag_path = join(FRAMEWORK_DIR, ".patching-done")

# # patch file only if we didn't do it before
# if not isfile(join(FRAMEWORK_DIR, ".patching-done")):
#     original_file = join(FRAMEWORK_DIR, "system", "STM32WBxx", "system_stm32wbxx.c")
#     patched_file = join("patches", "test-patch.patch")

#     assert isfile(original_file) and isfile(patched_file)

#     env.Execute("patch %s %s" % (original_file, patched_file))
#     # env.Execute("touch " + patchflag_path)


#     def _touch(path):
#         with open(path, "w") as fp:
#             fp.write("")

#     env.Execute(lambda *args, **kwargs: _touch(patchflag_path))