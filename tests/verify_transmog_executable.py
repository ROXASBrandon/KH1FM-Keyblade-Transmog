"""Verify native lookup against the actual supported executable (pefile required).

Usage: python tests/verify_transmog_executable.py /path/to/KH1.exe
Optional second argument: extracted KH1 asset directory.
"""
from pathlib import Path
import re
import struct
import sys
import pefile
ROOT=Path(__file__).resolve().parents[1]
p=pefile.PE(sys.argv[1]);image=p.get_memory_mapped_image()
source=(ROOT/'mods/keyblade-transmog/native/kh1_transmog.c').read_text()
# Item getter's RIP-relative pointer load: this catches the 0.1.0 pointer bug.
address=0x28F976
assert image[address:address+3]==bytes.fromhex('488b05')
items_ptr=address+7+struct.unpack_from('<i',image,address+3)[0]
used=int(re.search(r'uintptr_t items=\*\(uintptr_t \*\)\(game\+(0x[0-9A-F]+)\)',source).group(1),16)
assert used==items_ptr,(hex(used),hex(items_ptr))
assert image[0x4698D2]==106 and struct.unpack_from('<I',image,0x3EA388)[0]==540680280
assert image[0xD6A12:0xD6A1C]==bytes.fromhex('488935ff440d02488bc6')
assert image[0x286720:0x286730]==bytes.fromhex('48895c2418555641564883ec204c8bf2')
# Hook only the two .se format calls. The later .wpn calls remain untouched.
for site,load in [(0x286CCC,0x286CAC),(0x2871DA,0x2871BA)]:
    assert image[site]==0xE8
    assert site+5+struct.unpack_from('<i',image,site+1)[0]==0x51260
    assert image[load:load+3]==bytes.fromhex('4c8b0d')
    extension_ptr=load+7+struct.unpack_from('<i',image,load+3)[0]
    extension=struct.unpack_from('<Q',image,extension_ptr)[0]-p.OPTIONAL_HEADER.ImageBase
    assert image[extension:extension+4]==b'.se\0'
for site in [0x28743D]:
    assert image[site]==0xE8
    assert site+5+struct.unpack_from('<i',image,site+1)[0]==0x51260
if len(sys.argv)>2:
    assets=Path(sys.argv[2]);battle=(assets/'btltbl.bin').read_bytes()
    rows=[0]+list(range(5,22));items=[81]+list(range(86,103))
    models=['xw_ex_5010']+['xw_ex_'+str(n) for n in range(5020,5170,10)]+['xw_ex_5190','xw_ex_5200']
    for row,item,model in zip(rows,items,models):
        # Derive item metadata offset from the actual initializer instruction.
        init=0x28FA0B
        assert image[init:init+3]==bytes.fromhex("488d05")
        offset=init+7+struct.unpack_from("<i",image,init+3)[0]-0x2D22D40
        assert struct.unpack_from('<h',battle,offset+(item-1)*20+6)[0]==row+1,(item,row)
        assert battle[0x94F8+row*0x58:0x94F8+row*0x58+11]==model.encode()+b'\0'
        assert (assets/(model+'.wpn')).is_file()
print('PASS: item lookup and two sound-only format hooks match real executable; weapon metadata/assets verified')
