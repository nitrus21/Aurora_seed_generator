"""CYD 1.9.3 local audit: actual linked image and public native fixtures only."""
from pathlib import Path
import hashlib
import json
import os
import shutil
import struct
import subprocess
import sys
sys.dont_write_bytecode = True
ROOT=Path(__file__).resolve().parents[2]
OUT=ROOT/'tmp/audit-cyd-1.9.3-2026-09-15'
OUT.mkdir(parents=True,exist_ok=True)
env={k.upper():v for k,v in os.environ.items()}
env.pop('PSMODULEPATH',None)
def run(name,args,cwd=ROOT,timeout=300):
    result=subprocess.run([str(a) for a in args],cwd=cwd,env=env,
        capture_output=True,text=True,errors='replace',timeout=timeout)
    (OUT/(name+'.log')).write_text(result.stdout+result.stderr,encoding='utf-8')
    print(name+': '+('PASS' if result.returncode==0 else 'FAIL'),flush=True)
    if result.returncode:
        print((result.stdout+result.stderr)[-3000:]); raise SystemExit(result.returncode)
    return result.stdout

build=ROOT/'.pio/build/esp32-2432S028R'
main=(ROOT/'src/main.cpp').read_text()
setup=main[main.index('void setup() {'):]
assert setup.index('ui.emergencyWipeSecrets()') < setup.index('auroraCydBootCleanup()') < setup.index('ui.begin()')
config=(ROOT/'include/lv_conf.h').read_text()
assert '#define LV_USE_ASSERT_MALLOC 1' in config
assert '#define LV_ASSERT_HANDLER auroraCydUiFailure();' in config
print('PASS: startup wipe precedes UI; CYD LVGL allocation assertions use terminal cleanup',flush=True)
tc=Path.home()/'.platformio/packages/toolchain-xtensa-esp32/bin'
sys.path.insert(0,str(ROOT/'tools'))
from check_cyd_binary import verify
symbols=verify(build/'firmware.elf',build/'partitions.bin',tc/'xtensa-esp32-elf-nm.exe')
run('image-info',[sys.executable,'-m','esptool','--chip','esp32','image_info',build/'firmware.bin'])
(OUT/'symbols.txt').write_text(symbols)
for sym in ('panic_handler','__wrap_esp_panic_handler','auroraCydUiFailure','sha256_Transform','sha512_Transform'):
    run('asm-'+sym,[tc/'xtensa-esp32-elf-objdump.exe','-d','--disassemble='+sym,build/'firmware.elf'])
# Resolve Xtensa l32r literal pools: verify actual SHA calls target memzero and
# the SDK panic port loads the wrapped handler, not just a source-level intent.
raw=(build/'firmware.elf').read_bytes()
sections=[]
shoff=struct.unpack_from('<I',raw,32)[0]
shentsize,shnum=struct.unpack_from('<HH',raw,46)
for i in range(shnum):
    _,kind,_,addr,offset,size,*_=struct.unpack_from('<10I',raw,shoff+i*shentsize)
    if kind!=8: sections.append((addr,offset,size))
def word(address):
    for start,offset,size in sections:
        if start<=address and address+4<=start+size:
            return struct.unpack_from('<I',raw,offset+address-start)[0]
    raise AssertionError(hex(address))
addresses={line.split(maxsplit=2)[2]:int(line.split()[0],16) for line in symbols.splitlines()
    if len(line.split(maxsplit=2))==3 and all(c in '0123456789abcdefABCDEF' for c in line.split()[0])}
import re
for sym,target,minimum in [('sha256_Transform','memzero',13),('sha512_Transform','memzero',13),
                           ('panic_handler','__wrap_esp_panic_handler',1)]:
    asm=(OUT/('asm-'+sym+'.log')).read_text()
    loads=[int(m,16) for m in re.findall(r'l32r\s+a\d+,\s*([0-9a-f]+)',asm)]
    assert sum(word(addr)==addresses[target] for addr in loads)>=minimum,(sym,target)
print('PASS: linked SHA temporary wipes and wrapped panic call resolved through ELF literal pools',flush=True)

vswhere=Path(env['PROGRAMFILES(X86)'])/'Microsoft Visual Studio/Installer/vswhere.exe'
vs=subprocess.check_output([str(vswhere),'-latest','-products','*','-requires',
    'Microsoft.VisualStudio.Component.VC.Tools.x86.x64','-property','installationPath'],text=True,env=env).strip()
vcvars=Path(vs)/'VC/Auxiliary/Build/vcvars64.bat'
for line in subprocess.check_output(f'cmd /d /s /c ""{vcvars}" >nul && set"',text=True,env=env).splitlines():
    if '=' in line:
        k,v=line.split('=',1); env[k.upper()]=v
cl=shutil.which('cl',path=env['PATH'])
flags=['/nologo','/utf-8','/O2','/UNDEBUG','/EHsc','/std:c++20','/D_CRT_SECURE_NO_WARNINGS',
       '/D_CRT_NONSTDC_NO_DEPRECATE','/I'+str(ROOT/'tests/storage/stubs'),'/I'+str(ROOT/'include')]
run('storage-compile',[cl,*flags,ROOT/'tests/cyd_security/test_storage.cpp','/Fe:storage.exe'],OUT)
run('storage-errors',[OUT/'storage.exe'],OUT)
boot=(ROOT/'src/cyd_security.cpp').read_text()
first=boot.index('extern "C" bool auroraCydBootCleanup(void) {')
last=boot.index('\n}\n',first)+2
(OUT/'boot_function.inc').write_text(boot[first:last])
run('boot-compile',[cl,*flags,'/I'+str(OUT),ROOT/'tests/cyd_security/test_boot.cpp','/Fe:boot.exe'],OUT)
run('boot-faults',[OUT/'boot.exe'],OUT)
lib=ROOT/'.pio/libdeps/esp32-2432S028R/uBitcoin/src'
run('sha-compile',[cl,'/nologo','/O2','/UNDEBUG','/I'+str(lib),'/c','/TC',
    *[lib/'utility/trezor'/(n+'.c') for n in ('sha2','memzero','hmac')]],OUT)
run('sha-link',[cl,*flags,'/I'+str(lib),ROOT/'tests/crypto/test_sha_cleanup.cpp',
    'sha2.obj','memzero.obj','hmac.obj','/Fe:sha.exe'],OUT)
run('sha-residue',[OUT/'sha.exe'],OUT)
run('entropy',['cmd','/c',ROOT/'tests/native/run.cmd'])
run('codec',[sys.executable,ROOT/'tests/crypto/run.py'])
run('ui-cyd',[sys.executable,ROOT/'tests/ui/run.py','--cyd'])
run('ui-p4-regression',[sys.executable,ROOT/'tests/ui/run.py'])
run('published-assets',['powershell','-ExecutionPolicy','Bypass','-File',ROOT/'tests/release/verify.ps1','-ReleasedArtifactsOnly'])
identity={}
for name in ('firmware.bin','firmware.elf','partitions.bin','bootloader.bin'):
    data=(build/name).read_bytes()
    identity[name]={'bytes':len(data),'sha256':hashlib.sha256(data).hexdigest()}
    shutil.copy2(build/name,OUT/name)
sources={}
for folder in ('src','include','tools','targets/cyd'):
    for path in (ROOT/folder).rglob('*'):
        if path.is_file() and path.suffix in ('.cpp','.c','.h','.py','.csv'):
            sources[path.relative_to(ROOT).as_posix()]=hashlib.sha256(path.read_bytes()).hexdigest()
(OUT/'identity.json').write_text(json.dumps({'binaries':identity,'source_sha256':sources},indent=2))
print('Audit evidence:',OUT)
print('LIMIT: no flash, no physical crash/cold-boot extraction, native Mbed TLS differs from the CYD SDK.')
