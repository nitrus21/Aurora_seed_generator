"""Preserve IDF silicon-family constraints in both generated ESP image headers.

pioarduino's generic ElfToBin omits --min-rev-full / --max-rev-full. Re-create
each image and the final factory bundle using Espressif's tool.
No device is accessed and no flash command is used.
"""
from pathlib import Path
import re
import subprocess
import sys

Import("env")

# pioarduino's preprocessed linker templates omit the SDK header dependency.
# A changed silicon configuration must also regenerate the memory layout.
for script in ("memory.ld.in", "sections.ld.in"):
    env.Depends("$BUILD_DIR/esp-idf/esp_system/ld/" + script, "$BUILD_DIR/config/sdkconfig.h")


def revision_image(source, target, env):
    header = Path(env.subst("$BUILD_DIR")) / "config/sdkconfig.h"
    config = header.read_text(encoding="utf-8")
    minimum = int(re.search(r"#define CONFIG_ESP32P4_REV_MIN_FULL (\d+)", config)[1])
    maximum = int(re.search(r"#define CONFIG_ESP32P4_REV_MAX_FULL (\d+)", config)[1])
    expected = (100, 199) if env.subst("$PIOENV") == "waveshare-p4-rev1" else (300, 399)
    if (minimum, maximum) != expected:
        raise RuntimeError("AURORA: refusing image with mismatched silicon revision")
    esptool = Path(env.PioPlatform().get_package_dir("tool-esptoolpy")) / "esptool.py"
    subprocess.run([sys.executable, str(esptool), "--chip", "esp32p4", "elf2image",
        "--flash-mode", "dio", "--flash-freq", "80m", "--flash-size", "32MB",
        "--min-rev-full", str(minimum), "--max-rev-full", str(maximum),
        "-o", str(target[0]), str(source[0])], check=True)
    if Path(str(target[0])).name == env.subst("${PROGNAME}.bin"):
        # The platform's factory merge runs before this post action. Rebuild
        # that bundle too, so flashing it cannot bypass the revision limits.
        command = [sys.executable, str(esptool), "--chip", "esp32p4", "merge-bin",
            "-o", env.subst("$BUILD_DIR/${PROGNAME}.factory.bin"),
            "--flash-mode", "dio", "--flash-freq", "80m", "--flash-size", "32MB"]
        for offset, filename in env.get("FLASH_EXTRA_IMAGES", []):
            command.extend([str(offset), env.subst(filename)])
        command.extend([env.subst("$ESP32_APP_OFFSET") or "0x10000", str(target[0])])
        subprocess.run(command, check=True)
    return 0


for image in ("$BUILD_DIR/bootloader.bin", "$BUILD_DIR/${PROGNAME}.bin"):
    env.AddPostAction(image, env.VerboseAction(revision_image, "AURORA: enforce silicon revision in $TARGET"))
    env.AlwaysBuild(image)
