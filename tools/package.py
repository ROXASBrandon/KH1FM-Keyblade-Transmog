"""Build and package the seamless preview from this standalone repository."""
import hashlib
from pathlib import Path
import subprocess
import sys
import zipfile

ROOT = Path(__file__).resolve().parents[1]
if '--skip-build' not in sys.argv:
    subprocess.run([sys.executable, str(ROOT/'native/build.py')], check=True)
files = ['images/banner.gif', 'mod.yml', 'README.md', 'CREDITS.md',
         'CHANGELOG.md', 'DEVELOPMENT.md',
         'scripts/kh1_keyblade_transmog.lua', 'scripts/io_packages/kh1_transmog.dll',
         'native/seamless.c', 'native/asset_headers.h', 'native/build.py',
         'native/generate_headers.py', 'tests/native_test.c',
         'tests/verify_executable.py', 'tools/package.py']
output = ROOT/'downloads'
output.mkdir(exist_ok=True)
archive = output/'Keyblade-Transmog-v0.2.0-p7-preview.zip'
for name in files:
    payload = (ROOT/name).read_bytes()
    for private in ('/'+'mnt/', 'Users'+'/', 'Users'+chr(92)):
        for encoding in ('utf-8', 'utf-16le'):
            if private.encode(encoding) in payload:
                raise ValueError('Private path in '+name)
with zipfile.ZipFile(archive, 'w', compression=zipfile.ZIP_DEFLATED) as z:
    for name in files:
        info = zipfile.ZipInfo(name, date_time=(2026, 10, 7, 0, 0, 0))
        info.compress_type = zipfile.ZIP_DEFLATED
        info.external_attr = 0o100644 << 16
        z.writestr(info, (ROOT/name).read_bytes())
with zipfile.ZipFile(archive) as z:
    assert z.testzip() is None and z.namelist() == files
    for name in files:
        assert z.read(name) == (ROOT/name).read_bytes(), name
archives = sorted(output.glob('Keyblade-Transmog-v*-preview.zip'))
(output/'Keyblade-Transmog-SHA256SUMS.txt').write_text(''.join(
    hashlib.sha256(a.read_bytes()).hexdigest()+'  '+a.name+'\n' for a in archives))
print(archive)
