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
native=ROOT/'mods/keyblade-transmog/native/kh1_transmog.c'
if not native.exists(): native=ROOT/'native/kh1_transmog.c'
source=native.read_text()
# Item getter's RIP-relative pointer load: this catches the 0.1.0 pointer bug.
address=0x28F976
assert image[address:address+3]==bytes.fromhex('488b05')
items_ptr=address+7+struct.unpack_from('<i',image,address+3)[0]
used=int(re.search(r'uintptr_t items=\*\(uintptr_t \*\)\(game\+(0x[0-9A-F]+)\)',source).group(1),16)
assert used==items_ptr,(hex(used),hex(items_ptr))
assert image[0x4698D2]==106 and struct.unpack_from('<I',image,0x3EA388)[0]==540680280
assert image[0xD6A12:0xD6A1C]==bytes.fromhex('488935ff440d02488bc6')
assert image[0x286720:0x286730]==bytes.fromhex('48895c2418555641564883ec204c8bf2')
# Verify both vanilla sound-loading paths and the weapon sound-ID reader.
assert image[0x2954E9:0x2954F2]==bytes.fromhex("488b48208b493403ce")
assert image[0x297830:0x297835]==bytes.fromhex("f60104750f")
assert image[0x38ADC0:0x38ADC7]==bytes.fromhex("85c9750333c0c3")
assert image[0x2846AB:0x2846B1]==bytes.fromhex("8b15bf2c5e02")
assert image[0x28A750:0x28A75A]==bytes.fromhex("0fb747024123c4413bc4")
# Native idle paths compare the full DWORD animation ID against zero.
assert image[0x29F914:0x29F91B]==bytes.fromhex("83bf6401000000")
assert "tm_animation_idle(*(uint32_t *)(actor+0x164))" in source
assert image[0x2846B9:0x2846BE]==bytes.fromhex("e852600000")
assert 0x2846BE+struct.unpack_from('<i',image,0x2846BA)[0]==0x28A710
# Real pause handling can clear the mask before that final scheduler call.
assert image[0x17F4AE:0x17F4B4]==bytes.fromhex("891dbc7e6e02")
assert image[0x170FC2:0x170FC7]==bytes.fromhex("e889e10000")
assert "original_scheduler(group, mask | (swap_pending ? 1u : 0u))" in source
assert image[0x28CC10:0x28CC1A]==bytes.fromhex("4883ec288b1546a75d02")
assert image[0x2A23E0:0x2A23E6]==bytes.fromhex("4883ec2833d2")
assert image[0x2A17D8:0x2A17DD]==bytes.fromhex("4885c07473")
assert "#define CAMERA_SUBMIT_TASK 0x28CC10" in source
assert "#define MODEL_SUBMIT_TASK 0x2A23E0" in source
# Captured battle freeze: sound install waits for the native audio queue.
assert image[0x290A50:0x290A5A]==bytes.fromhex("48896c24104889742418")
assert image[0xD2830:0xD283A]==bytes.fromhex("4883ec288b05b2dc2602")
assert "#define AUDIO_QUEUE_TASK 0x290A50" in source
assert "#define ROOM_SUBMIT_TASK 0xD2830" in source
for site,target in [(0x2964C3,0x178A30),(0x178A41,0x290050),
                    (0xD285D,0x182230),(0xD287A,0x182250),(0x182287,0x19B4F0)]:
    assert image[site]==0xE8,hex(site)
    assert site+5+struct.unpack_from('<i',image,site+1)[0]==target,hex(site)
assert "on_present" not in source and "PRESENT_CALL" not in source
mask=0x2846B1+struct.unpack_from('<i',image,0x2846AD)[0]
assert mask==0x2867370 and "#define GAMEPLAY_PAUSE_MASK 0x2867370" in source
core=(ROOT/'mods/keyblade-transmog/native/transmog_core.h').read_text() if (ROOT/'mods/keyblade-transmog/native/transmog_core.h').exists() else (ROOT/'native/transmog_core.h').read_text()
assert "#define TM_SOUND_OFFSET 0x34" in core and "#define TM_SOUND_SIZE 4" in core
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
        sound_id=struct.unpack_from('<I',battle,0x94F8+row*0x58+0x34)[0]
        sound=(assets/(model+'.se')).read_bytes()
        ids=set();pos=0
        while (pos:=sound.find(b'SeSep',pos))>=0:
            ids.add(struct.unpack_from('<I',sound,pos+8)[0]);pos+=5
        assert all(sound_id+variant in ids for variant in range(0x23)),(model,hex(sound_id))
print('PASS: item/sound signatures, idle DWORD, native pause overwrite and final scheduler, audio dependency, room/camera/model tasks and missing-weapon render guard verified' + ('; 18 model/bank/35-hit-ID sets match' if len(sys.argv)>2 else ''))
