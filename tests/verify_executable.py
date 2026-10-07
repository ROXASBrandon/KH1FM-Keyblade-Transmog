"""Read-only supported-executable and locally extracted vanilla asset checks."""
from pathlib import Path
import sys,struct,pefile,re
root=Path(__file__).resolve().parents[1]
p=pefile.PE(sys.argv[1]);d=p.get_memory_mapped_image()
assert d[0x4698D2]==106 and struct.unpack_from('<I',d,0x3EA388)[0]==540680280
for site,target in [(0x2A1A08,0x1D6510),(0x2872FE,0x1D6750),
                    (0x287323,0x1E0080),(0x287280,0x296040),
                    (0x2BB3E9,0x2AE290),(0x2C2015,0x2AE290),
                    (0x2954F2,0x2961F0),(0x29619E,0x2961F0)]:
 # The native bank handoff is a tail jump; all other sites are calls.
 assert d[site]==(0xE9 if site==0x287280 else 0xE8) and site+5+struct.unpack_from('<i',d,site+1)[0]==target
assert d[0x1D6510]==0xE9 and 0x1D6515+struct.unpack_from('<i',d,0x1D6511)[0]==0x1D4070
assert d[0x28C35C:0x28C368]==bytes.fromhex('4c8b47508b53748b4f58ffd0')
assert d[0x2A180F:0x2A1816]==bytes.fromhex('48898598010000')
assert d[0x1D4D0A:0x1D4D12]==bytes.fromhex('488b461848894718')
assert d[0x28BC10:0x28BC1A]==bytes.fromhex('48895c24205556415448')
assert d[0xE2B20:0xE2B27]==bytes.fromhex('4c8b1df9840c02')
assert d[0x2954C8:0x2954CE]==bytes.fromhex('8b8f4c010000') # RDI actor for sound relay.
assert d[0x296187:0x29618D]==bytes.fromhex('8b8e4c010000') # RSI actor for sound relay.
s=(root/'native/seamless.c').read_text()
assert 'EquipProc' not in s and 'game+0x286720' not in s and 'on_scheduler' not in s
if len(sys.argv)>2:
 headers=(root/'native/asset_headers.h').read_text()
 for number in list(range(5010,5170,10))+[5190,5200]:
  name=f'xw_ex_{number}'
  b=(Path(sys.argv[2])/(name+'.wpn')).read_bytes();offset=struct.unpack_from('<I',b,8)[0]
  for data in [b[:16],b[offset:offset+56]]:assert '{'+','.join(hex(x) for x in data)+'}' in headers
  assert str(len(b)) in headers and b[offset:offset+4]==b'MENV'
  effects=struct.unpack_from('<I',b,4)[0]
  assert '{'+','.join(hex(x) for x in b[effects:effects+16])+'}' in headers
  count=struct.unpack_from('<I',b,effects+12)[0];assert count==9
  for i in range(count):
   entry=struct.unpack_from('<I',b,effects+16+i*32)[0]
   assert entry==0 or entry+16<=offset-effects
  n=struct.unpack_from('<I',b,effects+16+count*32)[0]
  assert effects+20+count*32+n*4<=offset
  for i in range(n):assert struct.unpack_from('<I',b,effects+20+count*32+i*4)[0]+16<=offset-effects
  sound=(Path(sys.argv[2])/(name+'.se')).read_bytes()
  assert '{'+','.join(hex(x) for x in sound[:16])+'}' in headers and str(len(sound)) in headers
print('PASS: graphics/effect/sound call targets, originating actor registers, native callback ABI, PC paths and vanilla WPN/SE headers/layouts.')
