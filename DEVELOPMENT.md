# Seamless graphics/effects/sound implementation

The stable helper invalidates the native cache and calls 0x286720. That releases
weapon graphics and clears actor +0x14C before asynchronously attaching a new
weapon. The seamless experiment avoids that routine entirely.

## Asset loading and ownership

Native loader 0x28BC10 creates a file task. A NULL destination follows the native
allocation path in 0x28C390; the completion callback receives byte count in ECX,
request ID in EDX and the loaded buffer in R8 (0x28C35C..0x28C366). Loading still
runs through normal application frames. No file task is manually pumped.

The callback requires exact supported vanilla WPN and MENV header prefixes and
file sizes. It verifies native resource registration through 0xE2B20 before
calling the same model initializer used by vanilla WPN completion: 0x1D6750.
MENV +0x20 becomes the graphics handle. PC initialization at 0x1DC7F0 looks up
file-relative HD metadata and creates a wrapper marked by 0x96969696 at mesh
+0x10; this marker is required before accepting the cache.

The engine owns the loaded buffers and their registered metadata. Cached assets
are borrowed only while the original registry record is present, headers and
graphics handle still match, and the original actor/model context remains valid.
This experiment makes only 36 requests and never retries, frees, or reuses a
stale cache. On room/actor changes it suspends overrides and retains the selected index.
After gameplay and a standard weapon attachment settle for 500 ms on the new
actor/model, both cached assets are validated before rebinding. There are no
additional file requests. If either resource unloads, it disables until restart.
Initial preload interruptions still disable; late callbacks cannot initialize
assets against a different actor.

## Draw substitution

The five-byte CALL at 0x2A1A08 targets 0x1D6510, which jumps to 0x1D4070. Its
arguments are actor model instance, matrices, render packet and actor pointer.
The packet's weapon-graphics pointer is +0x18, assembled at 0x2A180F. The renderer
copies that pointer into a draw record at 0x1D4D0A..0x1D4D0E; cached resources must
outlive the call. A hook replaces only that field for Sora while calling the
original renderer and restores the local packet immediately afterward. The
actor's weapon handle and every byte of its actual weapon record are untouched.

A NULL native packet weapon pointer remains NULL. The helper never forces a
weapon into a native hidden state. No scheduler or Present call is patched.
The original frame always executes. A modified stable-helper scheduler call is
rejected to prevent accidentally running both helpers together.

## Verification boundary

Private-memory Windows fixtures test 4,096 alternating draw substitutions,
unchanged attached weapon data and pause state, packet restoration, actual rel32
call/relay execution and ABI forwarding, 36 asynchronous loads, held/busy and
unfocused inputs, mid-swing input, Shift+Q, stale resource rejection, scene suspension/rebinding and retained selection/reset,
wrong callbacks, bad headers/sizes, missing ownership and timeout fallback.
Static checks validate the executable call chain and the two installed vanilla
asset headers. These do not prove final textures, geometry, frame pacing, resource
lifetime during live rendering or in-battle stability.


## Cosmetic effects and audio (p4)

Vanilla WPN completion initializes its effect section (raw + DWORD +4) with
0x1E0080 after model initialization. p4 follows that order, validates the nine
supported entries and section offsets, and retains the bank under the same
native resource ownership checks as its geometry. The calls at 0x2BB3E9 and
0x2C2015 target 0x2AE290(actor, bank, effect). Only Sora, only the actual weapon
bank, and only effect IDs present in the selected bank are redirected. The
native effect system owns new effect instances. Existing effects are allowed
to finish; p4 never deletes an active trail during a swing.

Eighteen extra native file requests preload one .se file for each cosmetic blade. Bank
installation uses 0x296040 and its normal scheduler/audio completion callback.
Groups 0x6F00..0x6F11 must be absent from KH's bank table at 0x2D53AB0 and the PC
audio registry rooted at 0x4D65D8. Native equipped/companion groups -3/-4/-5 are
untouched. Readiness requires all 35 expected blade-specific PC clips to be registered for each
cosmetic group. There is no polling loop, manual audio task pump or gameplay pause.

The two sound-base CALLs at 0x2954F2 and 0x29619E target 0x2961F0(sound ID, tag).
Their caller holds the actor in RDI or RSI. Dedicated relay prefixes copy that
actor into volatile R8 before jumping to the callback; nonvolatile registers
and native return values are preserved. The callback reads the equipped sound
base, preserves variants 0..34 and playback tag, then chooses the selected cosmetic base from the 18-entry supported weapon table only while that PC sound clip is still registered. Companions,
reset state, unsupported variants, missing clips and unrelated banks pass through.
No actual weapon record, parameter row or save record is written by these hooks.

Fixtures execute all five real rel32 relays, including actor extraction, test
630 sound variants, unsupported/stale/companion fallback, independent bank
collision refusal, asynchronous audio readiness and malformed effect layouts.
These checks do not prove audible playback, visible trails, native bank lifetime
across gameplay or stability in the real game; p4 remains private until live tests.


## Chest/event rendering

p2 used the same gameplay gate for input and display. Live state during the
reported reversion had cutscene byte 2, HUD float 0, and a standard weapon still
attached; that gate dropped the draw substitution. p4 uses a separate display
gate: the current bound actor/model, a validated native attachment, a standard
Sora parameter row (0 or 5..21) and a non-NULL submitted weapon pointer. HUD,
event/menu and cutscene flags continue to block Q and cosmetic sound/effect
hooks, but do not revert an otherwise valid visible blade. Warp/actor changes,
nonstandard event weapons and gummi still pass through. Fixtures cover this
separation; the actual chest animation still needs live confirmation.


## Native sound-bank layout correction (p4)

p3 incorrectly required 40 contiguous sound-base variants. Live inspection
showed Kingdom Key's native group has 35 hits at 0x2B10..0x2B32 plus five shared
swing clips at 0x2B0A..0x2B0E. Jungle King similarly retains those same shared
clips plus hits at 0x2CA0..0x2CC2. p4 requires the 35 hit variants; it leaves the
shared swing sounds untouched. Fixtures now use this observed layout and cover
variants 35..39 as pass-through. Sound readiness failures expose separate
scene/ownership/missing-clip reason codes through seamless_failure_reason.


## All-18 expansion (p5)

MODEL_COUNT and ALL_MODELS_MASK cover all 18 standard appearances. Preload
progress is bounded to 36 file requests: the 18 WPN model/effect banks, then
18 independent sound banks. Each completion sets its own bit, and READY is
entered only after both 18-bit masks are complete. The final bank therefore
cannot be skipped through a hardcoded two-blade completion condition.

Every room recovery revalidates all 36 borrowed resources before enabling draws.
Q advances modulo 18; Shift+Q resets the cosmetic index without changing actual
equipment. Cosmetic bases match the live vanilla table for all 18 rows. Header
metadata can be regenerated with generate_headers.py from a local extraction;
no full game assets are copied into the project or package.

All-18 fixtures validate ordered asynchronous requests, mask completion,
three complete Q cycles and wraparound, 4,096 render substitutions, 630 hit
variants, all 18 effect banks, chest/event rendering, native ABI relays,
scene recovery and unloaded/malformed/companion fallback. Static checks cover
all 18 WPN/SE pairs. Full live preload, all-18 visuals/effects and battle behavior
remain unverified until gameplay testing; the confirmed p4 pair is preserved.


## Death/retry resource ownership correction (p6)

p5 live death/retry cleared KH sound bookkeeping slots at 0x2D53AB0 while
all 18 WPN sources, all 18 SE sources, and the private PC audio groups remained
registered. The old slot check therefore permanently disabled a valid cache.
p6 validates SE headers, size, original resource ownership, and all 35 actual
hit clips in the PC audio registry instead. Sound dispatch still checks its
individual clip. Group reservation still checks both registries, so a cleared
KH slot cannot permit replacement of an existing private audio group.

The death fixture clears all KH bookkeeping slots, replaces Sora's actor, and
checks retained appearance, sound and trails after recovery with no additional
file requests. Missing resources and missing clips still disable recovery.
Live death/retry confirmation remains required. No saves are changed.


## Transition display continuity (p7)

The p6 READY phase waited for normal gameplay (including HUD visibility) and
500 ms of settled attachment before restoring rendering. Visible arrival
frames could therefore show the equipped blade while input recovery waited.
p7 allows render-only substitution in READY or SUSPENDED for the currently
published Sora actor, with readable actor/type, positive HP, standard equipment,
validated native attachment and cached resource, and a non-NULL submitted
weapon. It never dereferences the old bound actor. It does not change the
settle timer, input, trail or sound gates, or force native hidden blades visible.

Fixtures cover new-actor draws with warp active and HUD hidden, old-actor
submissions, hidden packets, zero HP, death/retry and missing resources.
Live area fade-in confirmation remains required.


## Published p7 validation

The author confirmed all final gameplay checks for p7: all 18 appearances,
battle Q spam, equipped stats/abilities, trails/hit sounds, chests, area
transitions without the equipped-blade flash, death/retry, and Shift+Q reset.
Earlier pending-live notes above record the development sequence; these checks
now passed on the author's setup. Other PCs and OpenKH installation remain
unverified. The public ZIP preserves the exact installed p7 DLL, build 2007.

The earlier 0.1.12 reload implementation is preserved in Git history and its
previous download. The active source and manifest use only the seamless helper;
do not load both versions in one process.
