"""Build the preview from its extracted source package; Python + ziglang 0.16.0."""
from pathlib import Path
import os
import subprocess
import sys
import tempfile

MOD=Path(__file__).resolve().parents[1]
NATIVE=MOD/'native'
TEST=MOD/'tests/transmog_core_test.c'
if not TEST.exists():
    TEST=MOD.parents[1]/'tests/transmog_core_test.c'
if not TEST.exists():
    raise SystemExit('Core test missing; extract the complete source package.')
cc=[sys.executable,'-m','ziglang','cc']
with tempfile.TemporaryDirectory(prefix='kh1-transmog-') as temp:
    test=Path(temp)/('core-test.exe' if os.name=='nt' else 'core-test')
    subprocess.run(cc+['-O2','-I',str(NATIVE),str(TEST),'-o',str(test)],check=True)
    subprocess.run([str(test)],check=True)
    if os.name=='nt':
        sound_test=TEST.with_name('transmog_native_test.c')
        hook_test=Path(temp)/'native-test.exe'
        subprocess.run(cc+['-O2','-I',str(NATIVE),str(sound_test),'-o',str(hook_test),
                          '-luser32','-lkernel32'],check=True)
        subprocess.run([str(hook_test)],check=True)
output=MOD/'scripts/io_packages/kh1_transmog.dll'
output.parent.mkdir(parents=True,exist_ok=True)
subprocess.run(cc+['-target','x86_64-windows-gnu','-O2','-shared','-s','-Wall','-Wextra',
                  str(NATIVE/'kh1_transmog.c'),'-o',str(output),'-luser32','-lkernel32'],check=True)
print(output)
