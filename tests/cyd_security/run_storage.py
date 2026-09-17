"""Focused storage test, no firmware build and no device I/O."""
from pathlib import Path
import os
import shutil
import subprocess
ROOT=Path(__file__).resolve().parents[2]
OUT=ROOT/'tmp/cyd-storage-regression'
OUT.mkdir(parents=True,exist_ok=True)
env={k.upper():v for k,v in os.environ.items()}
env.pop('PSMODULEPATH',None)
vswhere=Path(env['PROGRAMFILES(X86)'])/'Microsoft Visual Studio/Installer/vswhere.exe'
vs=subprocess.check_output([str(vswhere),'-latest','-products','*','-requires',
    'Microsoft.VisualStudio.Component.VC.Tools.x86.x64','-property','installationPath'],text=True,env=env).strip()
vcvars=Path(vs)/'VC/Auxiliary/Build/vcvars64.bat'
for line in subprocess.check_output(f'cmd /d /s /c ""{vcvars}" >nul && set"',text=True,env=env).splitlines():
    if '=' in line:
        k,v=line.split('=',1);env[k.upper()]=v
cl=shutil.which('cl',path=env['PATH'])
flags=['/nologo','/utf-8','/O2','/UNDEBUG','/EHsc','/std:c++20','/D_CRT_SECURE_NO_WARNINGS',
       '/D_CRT_NONSTDC_NO_DEPRECATE','/I'+str(ROOT/'tests/storage/stubs'),'/I'+str(ROOT/'include')]
for name,args in [('compile',[cl,*flags,ROOT/'tests/cyd_security/test_storage.cpp','/Fe:storage.exe']),
                  ('run',[OUT/'storage.exe'])]:
    result=subprocess.run([str(x) for x in args],cwd=OUT,env=env,capture_output=True,text=True)
    (OUT/(name+'.log')).write_text(result.stdout+result.stderr,encoding='utf-8')
    print(result.stdout+result.stderr,flush=True)
    result.check_returncode()
