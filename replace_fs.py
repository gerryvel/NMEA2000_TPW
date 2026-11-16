import os
import shutil

Import("env")
print("Replace MKSPIFFSTOOL with mklittlefs.exe")
env.Replace (MKSPIFFSTOOL = "mklittlefs.exe")


def copy_firmware_bin(source, target, env):
    project_dir = env.subst("$PROJECT_DIR")
    build_dir = env.subst("$BUILD_DIR")
    firmware_bin = os.path.join(build_dir, env.subst("$PROGNAME") + ".bin")
    output_dir = os.path.join(project_dir, "bin")
    os.makedirs(output_dir, exist_ok=True)
    shutil.copy2(firmware_bin, os.path.join(output_dir, os.path.basename(firmware_bin)))


env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", copy_firmware_bin)
