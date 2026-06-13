# PlatformIO post-build script: merge bootloader + partitions + app + LittleFS
# into one .bin.
# Usage:
#   pio run -t buildfs -e supermini   # build the LittleFS image first
#   pio run -t merge   -e supermini

Import("env")

import os
from os.path import join

# Offset of the LittleFS (spiffs subtype) partition, from partitions/plane_radar.csv.
FS_OFFSET = "0x3B0000"


def merge_firmware(source, target, env):
    build_dir = env.subst("$BUILD_DIR")
    progname = env.subst("${PROGNAME}")
    framework_dir = env.PioPlatform().get_package_dir("framework-arduinoespressif32")
    esptool = join(env.PioPlatform().get_package_dir("tool-esptoolpy"), "esptool.py")
    boot_app0 = join(framework_dir, "tools", "partitions", "boot_app0.bin")
    merged = join(build_dir, "firmware-merged.bin")
    mcu = env.BoardConfig().get("build.mcu", "esp32c3")
    flash_size = env.BoardConfig().get("upload.flash_size", "4MB")

    bootloader = join(build_dir, "bootloader.bin")
    partitions = join(build_dir, "partitions.bin")
    firmware = join(build_dir, f"{progname}.bin")
    littlefs = join(build_dir, "littlefs.bin")

    for path, label in (
        (bootloader, "bootloader.bin"),
        (partitions, "partitions.bin"),
        (boot_app0, "boot_app0.bin"),
        (firmware, f"{progname}.bin"),
    ):
        if not os.path.isfile(path):
            raise FileNotFoundError(f"Missing {label}: {path}")

    if not os.path.isfile(littlefs):
        raise FileNotFoundError(
            f"Missing littlefs.bin: {littlefs}\n"
            "Build the web UI filesystem image first:  pio run -t buildfs -e supermini"
        )

    cmd = [
        env.subst("$PYTHONEXE"),
        esptool,
        "--chip",
        mcu,
        "merge_bin",
        "-o",
        merged,
        "--flash_mode",
        "keep",
        "--flash_freq",
        "80m",
        "--flash_size",
        flash_size,
        "0x0",
        bootloader,
        "0x8000",
        partitions,
        "0xe000",
        boot_app0,
        "0x10000",
        firmware,
        FS_OFFSET,
        littlefs,
    ]
    print(f"Merging flash image -> {merged}")
    env.Execute(" ".join(f'"{c}"' if " " in c else c for c in cmd))
    return None


env.AddCustomTarget(
    name="merge",
    dependencies="${BUILD_DIR}/${PROGNAME}.bin",
    actions=env.Action(merge_firmware, "Merging flash image for web flasher"),
    title="Merge firmware",
    description="Create firmware-merged.bin (bootloader + partitions + app)",
)
