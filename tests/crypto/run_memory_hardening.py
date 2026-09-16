"""P4 pinned patches, fault rejection and optimized public-fixture regression.

Windows/MSVC is used for the isolated returned-stack test. Test outputs and
dependency copies stay under ignored tmp/; never reads or connects to a device.
"""
from pathlib import Path
import hashlib
import os
import shutil
import subprocess
import sys
import tempfile
sys.dont_write_bytecode = True

ROOT = Path(__file__).resolve().parents[2]
LIB = Path(os.environ.get("AURORA_TEST_UBITCOIN_LIB", str(ROOT /
    "targets/waveshare_p4/.pio/build/waveshare-p4-rev1/_deps/ubitcoin-src/src")))
COMMIT = "877542fdc16319dd92a7d2a679ea9dacce474bd2"
OUTPUT = Path(tempfile.mkdtemp(prefix="crypto-hardening-", dir=ROOT / "tmp"))
sys.path.insert(0, str(ROOT / "tools"))
import patch_ubitcoin_p4 as hardening
import prepare_p4_crypto_overlay as overlay


def run(command, **kwargs):
    result = subprocess.run(command, capture_output=True, text=True, errors="replace", **kwargs)
    print(result.stdout[-5000:] + result.stderr[-2000:], end="", flush=True)
    if result.returncode:
        raise RuntimeError("Command failed: " + str(command))


def digest_tree(directory):
    return {p.relative_to(directory).as_posix(): hashlib.sha256(p.read_bytes()).hexdigest()
            for p in directory.rglob("*") if p.is_file()}


def pristine_tree(destination):
    shutil.copytree(LIB, destination)
    changed = list(hardening.BASE_HASHES) + ["Hash.h", "Hash.cpp", "Conversion.cpp",
        "BitcoinCurve.cpp", "utility/trezor/rand.c"]
    for name in set(changed):
        raw = subprocess.check_output(["git", "-C", str(LIB.parent), "show", COMMIT + ":src/" + name])
        (destination / name).write_bytes(raw)


def patches():
    fixture = OUTPUT / "ubitcoin"
    pristine_tree(fixture)
    # The original rolled sources are the positive control: the test must find
    # residue there, or a zero-match result in the hardened build proves little.
    baseline = OUTPUT / "baseline"
    shutil.copytree(fixture, baseline)
    run([sys.executable, str(ROOT / "tools/patch_ubitcoin.py"), "--lib-root", str(fixture)])
    hardening.patch_tree(fixture)
    expected = digest_tree(fixture)
    run([sys.executable, str(ROOT / "tools/patch_ubitcoin.py"), "--lib-root", str(fixture)])
    hardening.patch_tree(fixture)
    assert digest_tree(fixture) == expected
    # A partial marked wipe patch must be rejected, without modifying any file.
    bad = OUTPUT / "corrupted"
    shutil.copytree(fixture, bad)
    target = bad / "utility/trezor/sha2.c"
    target.write_text(target.read_text().replace("memzero(&aurora_sha_work, sizeof(aurora_sha_work));", "/* missing wipe */", 1))
    before = digest_tree(bad)
    try:
        hardening.patch_tree(bad)
    except (RuntimeError, ValueError):
        pass
    else:
        raise AssertionError("Partial patch was accepted")
    assert digest_tree(bad) == before
    print("PASS: pristine -> V1 -> P4, repeat V1/P4 idempotence, corrupt patch rejected atomically.")
    return fixture, baseline


def compiler_environment():
    env = {k.upper(): v for k, v in os.environ.items()}
    vswhere = Path(env["PROGRAMFILES(X86)"]) / "Microsoft Visual Studio/Installer/vswhere.exe"
    vs = subprocess.check_output([str(vswhere), "-latest", "-products", "*", "-requires",
        "Microsoft.VisualStudio.Component.VC.Tools.x86.x64", "-property", "installationPath"], env=env, text=True).strip()
    vcvars = Path(vs) / "VC/Auxiliary/Build/vcvars64.bat"
    for line in subprocess.check_output(f'cmd /d /s /c ""{vcvars}" >nul && set"', env=env, text=True).splitlines():
        if "=" in line:
            key, value = line.split("=", 1)
            env[key.upper()] = value
    return env


def sha_tests(fixture, baseline, env):
    compiler = shutil.which("cl", path=env["PATH"])
    for name, sources, unrolled, residue in [
            ("baseline", baseline, False, True),
            ("hardened", fixture, False, False),
            ("hardened-unrolled", fixture, True, False)]:
        build = OUTPUT / (name + "-build")
        build.mkdir()
        flags = ["/nologo", "/O2", "/UNDEBUG", "/D_CRT_SECURE_NO_WARNINGS", "/I" + str(sources)]
        if unrolled:
            flags += ["/DSHA2_UNROLL_TRANSFORM"]
        run([compiler, *flags, "/c", "/TC", *[str(sources / "utility/trezor" / (n + ".c"))
            for n in ("sha2", "memzero", "hmac")]], cwd=build, env=env)
        run([compiler, *flags, "/EHsc", "/std:c++20", str(ROOT / "tests/crypto/test_sha_cleanup.cpp"),
             "sha2.obj", "memzero.obj", "hmac.obj", "/Fe:sha_cleanup.exe"], cwd=build, env=env)
        run([str(build / "sha_cleanup.exe")] + (["--expect-unpatched-residue"] if residue else []), cwd=build, env=env)


def overlay_tests():
    components = Path.home() / ".platformio/packages/framework-espidf/components"
    output = OUTPUT / "overlay"
    overlay.generate(components, output)
    before = digest_tree(output)
    overlay.generate(components, output)
    assert before == digest_tree(output)
    # The KDF adapter now relies directly on these HAL contracts. Changing
    # any of them must fail before an overlay can be emitted.
    pinned = OUTPUT / "sdk-pin-fixture"
    for name in overlay.SOURCES:
        target = pinned / name
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(components / name, target)
    for name in ("hal/sha_hal.c", "hal/esp32p4/include/hal/sha_ll.h",
                 "mbedtls/port/include/sha/sha_core.h"):
        target = pinned / name
        original = target.read_bytes()
        target.write_bytes(original + b"\n/* changed HAL */\n")
        rejected = OUTPUT / ("rejected-" + target.name)
        try:
            overlay.generate(pinned, rejected)
        except RuntimeError:
            pass
        else:
            raise AssertionError("Changed KDF HAL accepted: " + name)
        assert not rejected.exists(), "Rejected overlay must not be emitted"
        target.write_bytes(original)
    print("PASS: changed SHA HAL/header rejected before overlay emission.")
    # Exact generated release ordering is checked, including lock release.
    for name, peripheral, release in (("sha_core.c", "sha", "esp_crypto_sha_aes_lock_release();"),
                                      ("esp_aes_dma.c", "aes", "AES_RELEASE();")):
        text = (output / name).read_text()
        first = text.index("void esp_" + peripheral + "_release_hardware(")
        end = text.index("\n}", first)
        function = text[first:end]
        assert function.index("periph_clk(true)") < function.index("periph_clk(false)") < function.index(release)
    print("PASS: SDK overlay pinning/idempotence and peripheral reset-before-unlock ordering.")
    return output


def cleanup_path_tests(fixture, generated_overlay, env):
    build = OUTPUT / "cleanup-paths"
    build.mkdir()
    hd = (fixture / "HDWallet.cpp").read_text()
    functions = []
    for name in ("size_t HDPrivateKey::to_bytes(", "size_t HDPrivateKey::to_stream(", "int HDPrivateKey::xprv(char"):
        first = hd.index(name)
        end = hd.index("\n}", first) + 2
        functions.append(hd[first:end])
    (build / "hd_cleanup_functions.inc").write_text("\n".join(functions))
    md = (generated_overlay / "md.c").read_text()
    first = md.index("int mbedtls_md_hmac_finish(")
    end = md.index("\n}", first) + 2
    (build / "md_cleanup_function.inc").write_text(md[first:end])
    compiler = shutil.which("cl", path=env["PATH"])
    run([compiler, "/nologo", "/O2", "/UNDEBUG", "/EHsc", "/std:c++20", "/I" + str(build),
         str(ROOT / "tests/crypto/test_cleanup_paths.cpp"), "/Fe:cleanup_paths.exe"], cwd=build, env=env)
    run([str(build / "cleanup_paths.exe")], cwd=build, env=env)


if __name__ == "__main__":
    fixture, baseline = patches()
    generated_overlay = overlay_tests()
    env = compiler_environment()
    cleanup_path_tests(fixture, generated_overlay, env)
    sha_tests(fixture, baseline, env)
    print("Artifacts:", OUTPUT)
    print("LIMIT: native stack fixture; not a physical P4 RAM extraction or power-loss test.")
