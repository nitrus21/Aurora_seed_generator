"""Exercise the actual P4 CMake guards; no board access or provisioning."""
from pathlib import Path
import shutil
import subprocess

root = Path(__file__).resolve().parents[2]
cmake = shutil.which("cmake") or str(
    Path.home() / ".platformio/packages/tool-cmake/bin/cmake.exe")
check = root / "tests/security_policy/check.cmake"
required = ["ESP_SYSTEM_PANIC_SILENT_REBOOT",
            "COMPILER_OPTIMIZATION_ASSERTIONS_SILENT", "MBEDTLS_INTERNAL_MEM_ALLOC",
            "COMPILER_STACK_CHECK_MODE_STRONG", "APP_REPRODUCIBLE_BUILD"]
forbidden = ["ESP_DEBUG_OCDAWARE", "MBEDTLS_EXTERNAL_MEM_ALLOC",
             "MBEDTLS_DEFAULT_MEM_ALLOC", "MBEDTLS_CUSTOM_MEM_ALLOC",
             "SPIRAM_ALLOW_BSS_SEG_EXTERNAL_MEMORY", "SPIRAM_ALLOW_NOINIT_SEG_EXTERNAL_MEMORY",
             "ESP_LVGL_ADAPTER_LVGL_THREAD_STACK_IN_PSRAM", "ESP_COREDUMP_ENABLE_TO_FLASH",
             "ESP_COREDUMP_ENABLE_TO_UART", "SPIRAM_ENC_EXEMPT", "SECURE_BOOT_V2_FORCE_ENABLE_ECDSA",
             "COMPILER_STACK_CHECK_MODE_NONE", "COMPILER_STACK_CHECK_MODE_NORM",
             "COMPILER_STACK_CHECK_MODE_ALL", "APP_COMPILE_TIME_DATE"]
baseline = {key: "ON" for key in required}


def run(name, settings, expected, config=None):
    args = [cmake] + [f"-DCONFIG_{key}={value}" for key, value in settings.items()]
    if config:
        args.append(f"-DAURORA_SDKCONFIG_FILE={config}")
    result = subprocess.run(args + ["-P", str(check)], capture_output=True, text=True)
    if (result.returncode == 0) != expected:
        raise RuntimeError(f"{name}: unexpected result\n{result.stdout}\n{result.stderr}")
    if not expected and "AURORA P4" not in result.stderr:
        raise RuntimeError(f"{name}: failed outside the policy\n{result.stderr}")
    print("PASS:", name)


run("existing software-only profile (not a hardware security certificate)", baseline, True)
for key in required:
    run("required " + key, {**baseline, key: "OFF"}, False)
    run("missing " + key, {k: v for k, v in baseline.items() if k != key}, False)
for key in forbidden:
    run("reject " + key, {**baseline, key: "ON"}, False)
run("reject hardware ECDSA Secure Boot", {**baseline, "SECURE_BOOT": "ON",
    "SECURE_SIGNED_APPS_ECDSA_V2_SCHEME": "ON"}, False)
rsa = {**baseline, "SECURE_BOOT": "ON", "SECURE_SIGNED_APPS_RSA_SCHEME": "ON"}
run("Secure Boot requires revision bound", rsa, False)
run("ROM-764 rejects Secure Boot on rev3.0", {**rsa, "ESP32P4_REV_MIN_FULL": "300"}, False)
for revision in (100, 301, 302):
    run(f"RSA rev{revision} choice does not enable provisioning",
        {**rsa, "ESP32P4_REV_MIN_FULL": str(revision)}, True)
for profile in ("waveshare-p4-rev1", "waveshare-p4"):
    config = root / "targets/waveshare_p4/.pio/build" / profile / "config/sdkconfig.cmake"
    if config.exists():
        run("effective " + profile, {}, True, config)
    else:
        print("SKIP: effective configuration not built:", profile)
