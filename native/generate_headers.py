"""Regenerate supported asset metadata from a local vanilla extraction; no assets copied."""
from pathlib import Path
import struct,sys
root=Path(sys.argv[1]);out=Path(__file__).with_name('asset_headers.h')
numbers=list(range(5010,5170,10))+[5190,5200]
names=['Kingdom Key','Jungle King','Three Wishes','Fairy Harp','Pumpkinhead','Crabclaw','Divine Rose','Spellbinder','Olympia','Lionheart','Metal Chocobo','Oathkeeper','Oblivion','Lady Luck','Wishing Star','Ultima Weapon','Diamond Dust','One-Winged Angel']
bases=[0x2b10,0x2ca0,0x2cc8,0x36bb,0x36e3,0x370b,0x3733,0x2b60,0x2b88,0x375b,0x3783,0x380e,0x3836,0x2bb0,0x2bd8,0x2c00,0x385f,0x3887]
wpns=[(root/f'xw_ex_{n}.wpn').read_bytes() for n in numbers]
sounds=[(root/f'xw_ex_{n}.se').read_bytes() for n in numbers]
models=[struct.unpack_from('<I',b,8)[0] for b in wpns]
effects=[struct.unpack_from('<I',b,4)[0] for b in wpns]
s='/* Supported vanilla header metadata. Assets remain in the game. */\n#define MODEL_COUNT 18\n#define ALL_MODELS_MASK ((1u<<MODEL_COUNT)-1u)\n'
def strings(label,values):
 return 'static const char *const '+label+'[MODEL_COUNT]={'+','.join('"'+v+'"' for v in values)+'};\n'
def array(label,values,size):
 return 'static const unsigned char '+label+'[MODEL_COUNT]['+str(size)+']={\n'+''.join('{'+','.join(hex(x) for x in v)+'},\n' for v in values)+'};\n'
s+=strings('appearance_names',names)+strings('asset_names',[f'xw_ex_{n}.wpn' for n in numbers])+strings('sound_names',[f'xw_ex_{n}.se' for n in numbers])
s+=array('expected_wpn',[b[:16] for b in wpns],16)
s+=array('expected_model',[b[m:m+56] for b,m in zip(wpns,models)],56)
s+=array('expected_effect',[b[e:e+16] for b,e in zip(wpns,effects)],16)
s+=array('expected_sound',[b[:16] for b in sounds],16)
for label,values in [('expected_size',[len(b) for b in wpns]),('expected_sound_size',[len(b) for b in sounds]),('cosmetic_sound_base',bases)]:
 s+='static const uint32_t '+label+'[MODEL_COUNT]={'+','.join(str(x) for x in values)+'};\n'
out.write_text(s)
print('Generated metadata for 18 supported vanilla WPN/SE pairs.')
