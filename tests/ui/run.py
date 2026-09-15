"""Build the real LVGL 9 / AURORA UI with simulated hardware, then capture it."""
from pathlib import Path
import os
import shutil
import struct
import subprocess
import sys
import zlib

root = Path(__file__).resolve().parents[2]
cyd = "--cyd" in sys.argv
width, height = (320, 240) if cyd else (480, 800)
output = root / ("tmp/ui-cyd-native" if cyd else "tmp/ui-p4-native")
# Some desktop launchers provide both Path and PATH; MSBuild rejects duplicates.
env = {key.upper(): value for key, value in os.environ.items()}
vswhere = Path(env["PROGRAMFILES(X86)"]) / "Microsoft Visual Studio/Installer/vswhere.exe"
vs = subprocess.check_output([str(vswhere), "-latest", "-products", "*", "-requires",
    "Microsoft.VisualStudio.Component.VC.Tools.x86.x64", "-property", "installationPath"], env=env, text=True).strip()
vcvars = Path(vs) / "VC/Auxiliary/Build/vcvars64.bat"
compiler_env = subprocess.check_output(f'cmd /d /s /c ""{vcvars}" >nul && set"', env=env, text=True, errors="replace")
for line in compiler_env.splitlines():
    if "=" in line:
        key, value = line.split("=", 1)
        env[key.upper()] = value
output.mkdir(parents=True, exist_ok=True)
# Hold a handle while compiling: the P4 builder uses FileShare.None on this
# same lock, so it cannot replace managed LVGL sources underneath native tests.
lock_path = root / "targets/waveshare_p4/.pio/aurora-build.lock"
lock_path.parent.mkdir(parents=True, exist_ok=True)
if not cyd:
    try:
        build_lock = lock_path.open("a+b")
    except PermissionError:
        raise SystemExit("A P4 build is running. Wait before running the UI tests.")
objects = output / "objects"
objects.mkdir(exist_ok=True)
lvgl = root / (".pio/libdeps/esp32-2432S028R/lvgl" if cyd else "targets/waveshare_p4/managed_components/lvgl__lvgl")
if not (lvgl / "lvgl.h").exists():
    raise SystemExit("Configure the P4 firmware first to download pinned LVGL 9.5.0")
compiler = shutil.which("cl", path=env["PATH"])
assert compiler, "MSVC C/C++ compiler required"
includes = [root / "tests/ui", lvgl, root / "tests/ui/stubs", root / "tests/native/stubs",
            root / "include", root / "targets/waveshare_p4/main"]
if cyd: includes.insert(0,root / "tests/ui/cyd")
mbed = Path.home() / ".platformio/packages/framework-espidf/components/mbedtls/mbedtls"
includes += [mbed / "include", root / "tests/crypto"]
crypto_objects = [root / "tmp/crypto-native" / (name + ".obj") for name in
                 ["aes", "gcm", "md", "pkcs5", "sha256", "sha512", "platform_util", "constant_time", "cipher", "cipher_wrap", "block_cipher"]]
assert all(p.exists() for p in crypto_objects), "Run tests/crypto/run.py first"
common = ["/nologo", "/utf-8", "/DLV_CONF_INCLUDE_SIMPLE", "/DLV_KCONFIG_IGNORE",
          "/DAURORA_MEMORY_TEST", "/D_CRT_SECURE_NO_WARNINGS"] + [f'/I"{path}"' for path in includes]
if not cyd:
    # The C allocator must exercise the same P4 ownership registry as firmware.
    common += ["/DAURORA_BOARD_P4", "/DAURORA_NATIVE_TEST"]
# Both VG-Lite ports contain a vg_lite_matrix.c, but this software renderer uses
# neither. Exclude the disabled accelerator sources to keep object names unique.
sources = [p for p in lvgl.glob("src/**/*.c") if p.name != "vg_lite_matrix.c"]
sources += list((root / "src/assets").glob("*.c"))
sources += [root / "src/secure_lvgl_memory.c"]
if not cyd: sources += list((root / "targets/waveshare_p4/main/assets").glob("*.c"))
assert len({p.stem for p in sources}) == len(sources), "Duplicate C object names"
c_rsp = output / "compile-c.rsp"
config_time=max((root / "tests/ui/lv_conf.h").stat().st_mtime,
                (root / "include/lv_conf.h").stat().st_mtime,
                (root / "tests/ui/cyd/lv_conf.h").stat().st_mtime,
                Path(__file__).stat().st_mtime,
                (root / "include/secure_memory.h").stat().st_mtime,
                (root / "include/secure_lvgl_memory.h").stat().st_mtime)
changed = [p for p in sources if not (objects / (p.stem + ".obj")).exists() or
           max(p.stat().st_mtime, config_time) >
           (objects / (p.stem + ".obj")).stat().st_mtime]
c_rsp.write_text("\n".join(common + ["/c", "/std:c11", "/MP4", "/O1", "/w"] +
                         [f'"{p}"' for p in changed]), encoding="utf-8")
cpp_rsp = output / "link-ui.rsp"
cpp_rsp.write_text("\n".join(common + ([] if cyd else ["/DAURORA_BOARD_P4"]) + ["/std:c++20", "/EHsc", "/UNDEBUG",
    f'/FI"{root / "tests/crypto/config_select.h"}"',
    "/DAURORA_NATIVE_TEST", f'"{root / ("tests/ui/test_cyd.cpp" if cyd else "tests/ui/test_ui.cpp")}"',
    f'/Fe"{output / "ui_tests.exe"}"'] +
    [f'"{objects / (p.stem + ".obj")}"' for p in sources] +
    [f'"{p}"' for p in crypto_objects] + ["bcrypt.lib"]), encoding="utf-8")
commands = ([[compiler, "@" + str(c_rsp)]] if changed else []) + [[compiler, "@" + str(cpp_rsp)], [str(output / "ui_tests.exe")]]
for command in commands:
    result = subprocess.run(command, cwd=objects if command[0] == compiler else output,
                            env=env, capture_output=True, text=True, errors="replace", timeout=300)
    if result.returncode:
        (output / "failure.log").write_text(result.stdout + result.stderr, encoding="utf-8")
        errors = [line for line in result.stdout.splitlines() if "error" in line.lower() and "Possible failure" not in line]
        print("\n".join(errors) or result.stdout[-6000:])
        print(result.stderr[-6000:])
        raise SystemExit(result.returncode)
    print(result.stdout[-1000:], flush=True)

def chunk(kind, data):
    return struct.pack("!I", len(data)) + kind + data + struct.pack("!I", zlib.crc32(kind + data))

for source in output.glob("*.ppm"):
    raw = source.read_bytes().split(b"\n", 3)[3]
    assert len(raw) == width * height * 3
    stride=width*3
    rows = b"".join(b"\0" + raw[y * stride:(y + 1) * stride] for y in range(height))
    png = b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack("!IIBBBBB", width, height, 8, 2, 0, 0, 0))
    png += chunk(b"IDAT", zlib.compress(rows)) + chunk(b"IEND", b"")
    source.with_suffix(".png").write_bytes(png)
print(f"Captures: {output}")
