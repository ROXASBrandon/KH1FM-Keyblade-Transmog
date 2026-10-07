"""Build the experimental helper with Python and ziglang 0.16.0."""
from pathlib import Path
import os,subprocess,sys,tempfile
root=Path(__file__).resolve().parents[1]
cc=[sys.executable,'-m','ziglang','cc']
if os.name=='nt':
 with tempfile.TemporaryDirectory() as temp:
  exe=Path(temp)/'native-test.exe'
  subprocess.run(cc+['-O2','-I',str(root/'native'),str(root/'tests/native_test.c'),'-o',str(exe),'-luser32','-lkernel32','-lbcrypt'],check=True)
  subprocess.run([str(exe),str(Path(sys.argv[1]) if len(sys.argv)>1 else root.parents[1]/'mods/keyblade-of-heart/generated/xw_ex_5010.wpn')],check=True)
dll=root/'scripts/io_packages/kh1_transmog.dll';dll.parent.mkdir(parents=True,exist_ok=True)
subprocess.run(cc+['-target','x86_64-windows-gnu','-O2','-shared','-s','-Wall','-Wextra',str(root/'native/seamless.c'),'-o',str(dll),'-luser32','-lkernel32','-lbcrypt'],check=True)
print(dll)
