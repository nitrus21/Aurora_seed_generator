"""CYD 1.9.3: reuse the pinned portable SHA/HD wipe supplement.

The supplement's P4 hardware calls remain conditional; CYD uses volatile wipes.
The linked image is inspected separately by tests/cyd_security/run.py.
"""
Import("env")
from pathlib import Path
import sys
sys.dont_write_bytecode = True
sys.path.insert(0, str(Path(env.subst("$PROJECT_DIR")) / "tools"))
from patch_ubitcoin_p4 import patch_tree
patch_tree(Path(env.subst("$PROJECT_LIBDEPS_DIR")) / env.subst("$PIOENV") / "uBitcoin/src")

def verify_image(source, target, env):
    sys.path.insert(0, str(Path(env.subst("$PROJECT_DIR")) / "tools"))
    from check_cyd_binary import verify
    toolchain=Path(env.PioPlatform().get_package_dir("toolchain-xtensa-esp32")) / "bin"
    nm=toolchain / ("xtensa-esp32-elf-nm.exe" if sys.platform=="win32" else "xtensa-esp32-elf-nm")
    build=Path(env.subst("$BUILD_DIR"))
    verify(build / "firmware.elf",build / "partitions.bin",nm)

env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", verify_image)
