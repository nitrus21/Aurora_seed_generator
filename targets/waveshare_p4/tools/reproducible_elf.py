"""Remove host-specific DWARF before esptool embeds the ELF SHA-256.

Windows and Linux can emit different non-loadable debug paths even when every
loadable byte is identical. Stripping only debug sections after linking keeps
the symbol table for address-level diagnosis and gives esptool one canonical
ELF to identify in the application descriptor.
"""

from pathlib import Path
import os
import subprocess

Import("env")


def strip_host_debug(target, source, env):
    elf = Path(str(target[0]))
    toolchain = Path(env.PioPlatform().get_package_dir("toolchain-riscv32-esp"))
    executable = "riscv32-esp-elf-objcopy.exe" if os.name == "nt" else "riscv32-esp-elf-objcopy"
    objcopy = toolchain / "bin" / executable
    if not objcopy.is_file():
        raise RuntimeError("AURORA: objcopy is unavailable for ELF normalization")
    subprocess.run([str(objcopy), "--strip-debug", str(elf)], check=True)
    return 0


# This post action completes before firmware.bin, which depends on firmware.elf,
# is generated. The embedded app_elf_sha256 therefore describes this exact,
# canonical ELF rather than host-specific DWARF metadata.
env.AddPostAction(
    "$BUILD_DIR/${PROGNAME}.elf",
    env.VerboseAction(strip_host_debug, "AURORA: strip host-specific ELF debug sections"),
)
