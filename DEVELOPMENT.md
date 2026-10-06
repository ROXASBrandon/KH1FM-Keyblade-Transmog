# Development and release checks

Use Python 3 and `ziglang==0.16.0`. Run `python native/build.py` to test the
cosmetic logic and build the Windows x64 DLL. Windows builds also run the
native capture/apply/reset integration test against isolated memory; the
test never opens or patches a game process.

To inspect the supported executable locally, install `pefile`, then run
`python tests/verify_transmog_executable.py <KH1.exe> <extracted-KH1-assets>`.
Executable signatures, vanilla sound loaders, the four-byte sound-ID reader, item metadata, and
all 18 model/sound-bank/35-hit-ID sets must match. These proprietary files are not distributed.

Before updating downloads, run `python tools/package.py`. The package uses
an explicit file list, checks for private paths, runs ZIP integrity checks,
and writes the SHA-256 checksum alongside the preview ZIP.

## Gameplay status

- Confirmed: Q appearance cycling and room-transition persistence.
- Live inspection: equipped Jungle King remained equipped with another model;
  all weapon parameter bytes after the filename matched the original checksum.
- Automated: all 324 equipped/appearance model and sound-ID combinations, unchanged
  combat fields, restoration, input edge detection, and Windows native
  capture/apply/reset behavior.
- Pending: clean restart on 0.1.10, restored hit audio, Shift+Q, other equipped
  weapons, death/continue, fresh OpenKH install, removal, and another PC.

Keep this build labeled preview until those gameplay checks pass. A compiled
or synthetic-test result does not establish audible in-game behavior.

## Empty-hand attack regression

The local dumps record access violations at 0x2A7700 (attack writes weapon
+0x41) and 0x29346F (command-menu cleanup writes weapon +0x42). Both have a
null weapon. The cosmetic equip routine clears Sora +0x14C before asynchronous
attachment. Actor bit 2 does not stop command-menu cleanup, so 0.1.4/0.1.5
are not crash fixes. Weapon +0x41 is a persistent hit counter, not readiness.

Version 0.1.6 held the whole application frame and manually pumped asset jobs.
Live inspection of the freeze showed a detached weapon, loader state 3, pending
file tasks and no worker job. Resource dependencies still needed normal frames.
That approach is removed.

Version 0.1.7 always calls the normal application frame. During an owned reload,
it sets bit 0 of the native gameplay task mask at 0x2867370. The native scheduler
at 0x284690 reads it at 0x2846AB; the task filter at 0x28A750 excludes tasks whose
flags lack that mask. Native pause flows at 0x17F2CC use this same mask. The
command-menu task is created with flags 0 and is excluded while paused.

Windows fixtures emulate 324 reloads: application callbacks and load progression
continue while gameplay tasks remain excluded. The pause bit is released after
a valid attached weapon and initialized graphics, preserving other mask bits.
This is still a fixture, not a live proof. Check Q, attack/cleanup, frame progress,
sounds, room transitions and reset before publication.

## Mid-swing Q regression

The latest dump faults at 0x2A76A9 while reading weapon +0x50 through a null
pointer. Broad actor action state +0x70 remains zero during attacks, so that
guard alone was insufficient. Native idle paths compare the DWORD animation
ID at actor +0x164 to zero. Version 0.1.8 requires this idle ID, clear native
action-eligibility flags, and 300 ms of uninterrupted readiness. Busy Q edges
are consumed rather than queued. Synthetic tests reject nonzero full-width
animation IDs and reset the idle timer on any busy frame. Live reproduction
remains pending; live sampling confirmed idle animation 0 and attack 200.

## Native pause overwrite regression

Version 0.1.8 still crashed at 0x2A7700. Live inspection confirms that the
player pointer matches the guarded actor; the actor slot can change between
sessions. Read-only swing sampling confirmed idle ID 0 and attack ID 200.

The normal frame calls pause handling at 0x170FC2 and the native scheduler at
0x170FD0. Pause handling writes the mask at 0x17F4AE, after our frame hook and
before task iteration. The early mask could therefore be erased. Version
0.1.9 hooks the five-byte call at 0x2846B9, passing mask | 1 to the original
0x28A710 only during a pending cosmetic reload. The native mask global remains
untouched. The rel32 call reaches an execute/read relay allocated near the
executable; both hooks remain pinned until exit. Unsupported or competing
call-site modifications disable installation before any swap.

Live task inspection confirms actor update (0x298940) and command cleanup
(0x2937F0) both have task flags 0, so the final mask excludes both. Loading and
service tasks retain 0xFFFF flags and continue. Regression fixtures clear the
early global before every scheduler invocation, emulate 324 delayed reloads,
preserve unrelated mask bits, and execute the real patched rel32 call/relay
in private test memory. Exported build, swap and paused-scheduler counters allow
read-only verification of the installed helper. Gameplay retesting is pending.

## Black-frame regression

Live 0.1.9 diagnostics recorded 24 swaps and 145 protected scheduler invocations.
The user reported no crashes during that test, but black flashes between swaps.
The scheduler mask also excludes scene-related tasks; application frames still
presented the resulting incomplete scene. Version 0.1.10 retains the same reload
protection and hooks only the two presentation call sites at 0x104941 and
0x10497A, both `call [rax+0xB0]`. These invoke IDXGISwapChain1::Present1 through
vtable slot 22 with the swap-chain pointer, interval, flags and parameters.
See [Microsoft's Present1 reference](https://learn.microsoft.com/en-us/windows/win32/api/dxgi1_2/nf-dxgi1_2-idxgiswapchain1-present1).

During pending reloads, display calls return S_OK without presenting a blank
frame; DXGI_PRESENT_TEST status queries still call the original method. The
last displayed frame stays visible until attachment completes. This keeps the
brief pause rather than re-enabling unsafe gameplay tasks. Both call hooks use
the same sealed near relay as the scheduler hook. All signatures and page
protections are validated before mutation, and shared-page protections are
restored only after every call is patched. Native fixtures execute both actual
patched call sites, verify argument/HRESULT forwarding, held presentations,
status-query passthrough, resumption, and original execute/read page protection.
Live black-flash and repeated-swap testing remain pending.

## Withdrawal of 0.1.10

The next live run crashed at 0x10AA29, a null read of mapped graphics timing
data after a graphics query. The call stack includes presentation bookkeeping
(0x104AC8). This is a different path from the weapon null-pointer reports.
Skipping Present1 while the engine continued its GPU bookkeeping was unsafe.
The installed helper and prepared source now match the exact 0.1.9 archive;
presentation hooks are removed. Its DLL hash matches the pre-0.1.10 backup.
The withdrawn archive/source/dump are retained privately for diagnosis.
Black-frame removal remains unresolved, and 0.1.9 is still a preview.

## 0.1.11 camera/model allowlist

The native model task wrapper at 0x2A23E0 iterates the model task group. Its
per-object callback at 0x2A1320 submits a rendering callback; the mesh assembly
path at 0x2A1630 resolves actor +0x14C at 0x2A17D3 and checks for null at
0x2A17D8 before the weapon fields are read. Missing weapons take 0x2A1850,
setting the weapon graphics pointer to null while continuing body submission.
The camera task at 0x28CC10 copies camera matrices and calls 0x18C620 to copy
renderer state. Its task is also excluded by the blanket reload mask.

Version 0.1.11 keeps the original scheduler reload mask, temporarily adding
bit 0 only to these two task records for one scheduler invocation. The original
iterator preserves task ordering and all native mask bits. After it returns,
the helper clears only its owned bit when the node still has the same callback.
Traversal is bounded at 256 nodes and eight owned records; unreadable or unknown
tasks never enter the allowlist. Pre-existing task bits remain unchanged.
No Present1 call is patched or suppressed. The likely black-frame cause is
missing camera/model submission; final visual behavior requires live testing.

Native fixtures emulate 324 delayed loads: camera/model submission and service
callbacks run, while actor, command-cleanup and unknown callbacks remain blocked
until attachment. Tests verify flag restoration and preservation of native
pause masks and pre-existing bits. Executable checks verify both task signatures
and the native missing-weapon branch. These checks do not establish live visuals.


## 0.1.12 captured audio dependency and room submission

The 0.1.11 battle freeze was captured with the loader at flags 3, Sora's weapon
handle zero, no file requests, and one sound-install task at stage 0. The native
audio queue at 0x2D371D0 had a companion-owned record at stage 4. Sound callback
0x296480 calls 0x178A30, which first calls 0x290050. That helper checks the four
queue records and reports busy until each stage byte becomes zero. The queue
updater 0x290A50 had task flags 0 and was excluded by the reload mask, so this
dependency could never complete. Allowing that updater lets native sound work
progress without enabling actor or command-cleanup callbacks. Its code uses
actor model/sound references; it does not access the detached weapon handle.

Actors remained visible in the captured freeze, but the room disappeared. The
room task at 0xD2830 was also excluded. Its native call chain copies scene state,
runs 0x182230 for scene matrix setup, and calls 0x182250 -> 0x19B4F0 to assemble
room geometry. It also updates room texture groups and background music. The
allowlist now includes this task, retaining normal engine submission and GPU
presentation. It does not enable room scripts, actor simulation, attack logic,
command cleanup, or other unknown tasks. Temporary bits still follow original
node order and are restored after the scheduler returns.

Private native fixtures now model the captured dependency: without the audio
allowance the queue remains busy and attachment cannot complete, even while
service tasks run; with the allowance the queue drains and 324 delayed loads
finish. Camera/model/room callbacks run while attack/cleanup/unknown callbacks
stay excluded. Executable checks verify the queue-busy call chain and the room
submission calls. These fixtures do not prove live battle stability or visuals.


## Final 0.1.12 gameplay validation — 2026-10-06

The author reported stable battle swaps and then confirmed all final checks
passed: cycling all 18 appearances in battle; equipping a different Keyblade
while retaining its stats/abilities; Shift+Q reset; room transitions; save/reload;
a fresh restart; and matching hit sounds. The remaining brief gameplay pause
during a swap is intentional while native model/sound loading completes. These
reports supersede the earlier pending-validation notes for the current build.
They do not establish compatibility on other PCs or OpenKH installation.
