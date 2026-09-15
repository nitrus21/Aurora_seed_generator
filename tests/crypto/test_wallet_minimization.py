"""Compile the real redundant-descriptor branch with public-only fixtures.

This is a bounded storage-policy regression, not a crypto-vector or firmware
test: the exact production branch is compiled for P4 and CYD, using the real
WalletOutput declaration and a descriptor wrapper that counts private copies.
"""
from pathlib import Path
import os
import shutil
import subprocess
import tempfile


root = Path(__file__).resolve().parents[2]
source = (root / "src/wallet.cpp").read_text(encoding="utf-8")
start = source.index("#if defined(AURORA_BOARD_P4)\n  // No P4 display or export")
end = source.index("#endif", start) + len("#endif")
branch = source[start:end]
assert "secureZero(out.privateDescriptor" in branch
assert "wrapDescriptor(kind, out.privateWif, out.privateDescriptor" in branch

env = {key.upper(): value for key, value in os.environ.items()}
vswhere = Path(env["PROGRAMFILES(X86)"]) / "Microsoft Visual Studio/Installer/vswhere.exe"
vs = subprocess.check_output([str(vswhere), "-latest", "-products", "*", "-requires",
    "Microsoft.VisualStudio.Component.VC.Tools.x86.x64", "-property", "installationPath"],
    env=env, text=True).strip()
vcvars = Path(vs) / "VC/Auxiliary/Build/vcvars64.bat"
for line in subprocess.check_output(f'cmd /d /s /c ""{vcvars}" >nul && set"',
        env=env, text=True, errors="replace").splitlines():
    if "=" in line:
        key, value = line.split("=", 1)
        env[key.upper()] = value
compiler = shutil.which("cl", path=env["PATH"])
assert compiler, "MSVC compiler required"
output = Path(tempfile.mkdtemp(prefix="wallet-minimization-", dir=root / "tmp"))
harness = output / "branch.cpp"
harness.write_text(r'''
#include <cassert>
#include <cstdio>
#include <cstring>
#include "wallet.h"
#include "secure_memory.h"
static unsigned privateCopies;
static bool wrapDescriptor(AddressKind, const char *wif, char *out, size_t size) {
    ++privateCopies;
    return snprintf(out, size, "fixture(%s)", wif) > 0;
}
static bool populatePrivateDescriptor(AddressKind kind, WalletOutput &out) {
''' + branch + r'''
    return true;
cleanup:
    return false;
}
int main() {
    for(unsigned kind=0; kind<4; ++kind) {
        WalletOutput out{};
        // These deliberately invalid key strings cannot hold wallet funds.
        strcpy(out.privateWif, "public-test-fixture-not-a-key");
        strcpy(out.accountXpub, "public-test-fixture-not-an-xpub");
        memset(out.privateDescriptor, 0xA5, sizeof(out.privateDescriptor));
        privateCopies=0;
        assert(populatePrivateDescriptor(static_cast<AddressKind>(kind), out));
        assert(!strcmp(out.privateWif, "public-test-fixture-not-a-key"));
        assert(!strcmp(out.accountXpub, "public-test-fixture-not-an-xpub"));
#if defined(AURORA_BOARD_P4)
        assert(privateCopies==0);
        for(char byte:out.privateDescriptor) assert(byte==0);
#else
        assert(privateCopies==1);
        assert(!strcmp(out.privateDescriptor, "fixture(public-test-fixture-not-a-key)"));
#endif
        secureZero(&out, sizeof(out));
    }
    printf("%zu\n", sizeof(WalletOutput));
}
''', encoding="utf-8")
includes = [root / "tests/ui/stubs", root / "tests/native/stubs", root / "include"]
sizes = []
for profile in ("p4", "cyd"):
    binary = output / f"{profile}.exe"
    args = [compiler, "/nologo", "/utf-8", "/EHsc", "/std:c++20", "/O2", "/UNDEBUG",
            "/D_CRT_SECURE_NO_WARNINGS", "/DAURORA_NATIVE_TEST"]
    if profile == "p4":
        args.append("/DAURORA_BOARD_P4")
    args += ["/I" + str(path) for path in includes]
    args += [str(harness), "/Fe" + str(binary)]
    subprocess.run(args, cwd=output, env=env, check=True, capture_output=True, text=True)
    sizes.append(subprocess.check_output([str(binary)], cwd=output, env=env, text=True).strip())
assert sizes[0] == sizes[1], "WalletOutput layout differs between P4 and CYD"
print("PASS: compiled production P4 branch creates no duplicate private descriptor; CYD still creates it for all four address kinds")
print(f"PASS: unchanged shared WalletOutput layout ({sizes[0]} bytes on this native host), WIF and xpub unchanged")
print(f"Public fixture artifacts: {output}")
