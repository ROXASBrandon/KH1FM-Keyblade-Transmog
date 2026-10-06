"""Build and package the preview from this standalone repository."""
import hashlib
from pathlib import Path
import subprocess
import sys
import zipfile

ROOT=Path(__file__).resolve().parents[1]
subprocess.run([sys.executable,str(ROOT/'native/build.py')],check=True)
files=['images/banner.svg','mod.yml','README.md','CREDITS.md','CHANGELOG.md',
       'scripts/kh1_keyblade_transmog.lua','scripts/io_packages/kh1_transmog.dll',
       'native/kh1_transmog.c','native/transmog_core.h','native/build.py',
       'tests/transmog_core_test.c','tests/transmog_sound_hook_test.c']
output=ROOT/'downloads'
output.mkdir(exist_ok=True)
archive=output/'Keyblade-Transmog-v0.1.2-preview.zip'
with zipfile.ZipFile(archive,'w',compression=zipfile.ZIP_DEFLATED) as z:
    for name in files:
        payload=(ROOT/name).read_bytes()
        for private in ('/mnt/','Users/roxas','Users'+chr(92)+'roxas'):
            for encoding in ('utf-8','utf-16le'):
                if private.encode(encoding) in payload:
                    raise ValueError('Private path in '+name)
        info=zipfile.ZipInfo(name,date_time=(2026,10,6,0,0,0))
        info.compress_type=zipfile.ZIP_DEFLATED
        info.external_attr=0o100644<<16
        z.writestr(info,payload)
with zipfile.ZipFile(archive) as z:
    assert z.testzip() is None and z.namelist()==files
(output/'Keyblade-Transmog-SHA256SUMS.txt').write_text(
    hashlib.sha256(archive.read_bytes()).hexdigest()+'  '+archive.name+'\n')
print(archive)
