# Changelog

## 0.2.0-p7 — seamless all-18 preview

- Preload all 18 cosmetic model/effect banks and 18 independent audio banks once.
- Replace reload-based switching with render submission substitution. The equipped
  weapon remains attached; combat fields and native pause masks remain unchanged.
- Allow instant switching during swings and battle. Held Q does not cycle;
  Shift+Q restores the equipped appearance and effects.
- Redirect new trails and the 35 blade-specific hit variants to the selected blade.
  Preserve native shared swing clips, companions, and unsupported sound variants.
- Keep cosmetic rendering during chest events and visible area fade-in without
  waiting for HUD/input readiness. Native hidden blades stay hidden.
- Preserve selection and rebind the current Sora actor after area changes and
  death/retry. Validate live resources and PC audio ownership even when death
  clears KH sound bookkeeping slots.
- Final gameplay checks passed on the author's setup, including all appearances,
  battle Q spam, chests, transitions, death/retry, and Shift+Q reset. Other PCs
  and OpenKH installation remain unverified; this remains a preview.
- Ship the exact tested p7 DLL (native build 2007). Keep the 0.1.12 download
  available for rollback.

## 0.1.12 — drain companion audio queue and submit room geometry

- Fix a captured battle reload deadlock: sound-bank installation waits for the
  companion audio command queue, whose updater was excluded by the reload mask.
- Allow the native audio command updater and room submission task during reload,
  alongside camera/model submission. Preserve native order and other pause bits.
- Keep attacks, command cleanup, AI and unknown tasks blocked until attachment.
- Regression fixtures reproduce the blocked audio dependency and verify 324
  reloads with audio progression, room submission and combat task exclusion.
- Final gameplay checks passed on the author's setup: all 18 appearances in
  battle, equipped stats/abilities, hit sounds, Shift+Q reset, room transitions,
  save/reload, and a fresh restart. A brief gameplay pause per swap remains
  expected. Other PCs and OpenKH installation remain unverified.

## 0.1.11 — keep camera/model submission during protected reload

- Retain the final scheduler crash guard and normal GPU presentation.
- Temporarily allow only the verified camera-matrix and model-submission tasks
  through the reload mask, preserving native task order and unrelated flags.
  The native model path checks for a missing weapon before weapon submission.
- Keep attack, command cleanup, simulation and unknown tasks blocked during reload.
- Regressions cover 324 delayed reloads with camera/model/service progression,
  blocked attack/cleanup/unknown tasks, flag restoration and native pause masks.
- Black-flash removal and crash stability remain pending live gameplay tests.

## 0.1.10 — withdrawn after graphics crash

- Skipping Present1 calls to retain the visible frame caused a new crash in
  graphics timing at supported-executable RVA 0x10AA29.
- Remove the presentation hooks and return to the exact 0.1.9 helper/source.
  Preserve the final scheduler reload protection and normal graphics bookkeeping.
- Black flashes during reload remain unresolved; no stable release is claimed.

## 0.1.9 — enforce pause at the final scheduler call

- Fix the early-mask race: normal pause handling can overwrite the reload mask
  after the application-frame hook and before gameplay task iteration.
- Hook the verified final scheduler call and add the reload bit to its argument;
  leave the native mask global untouched and preserve unrelated mask bits.
- Keep application/loading frames running and the 300 ms idle requirement.
- Regression tests reproduce native mask resets through 324 delayed reloads and
  execute the actual patched call/near relay. Live task inspection confirms the
  actor and command-cleanup tasks are excluded by the reload bit.
- Add build/swap/paused-scheduler diagnostic counters. Live crash retest pending.

## 0.1.8 — accept Q only after stable idle

- Reject Q during non-idle animation even when the broad action state is zero.
- Require 300 ms of uninterrupted idle readiness and clear native action flags.
- Consume busy Q presses without queueing a later swap; preserve the native
  reload pause and cosmetic model/sound behavior.
- Add regressions for full-width animation IDs, eligibility flags, stable-idle
  timing, and interrupted idle windows. Live mid-swing crash retesting pending.

## 0.1.7 — native gameplay pause, normal loading

- Remove 0.1.6 whole-frame suppression and manual resource/worker pumping.
  Live feedback confirmed these could freeze a pending weapon load.
- Use bit 0 of the native gameplay scheduler mask at 0x2867370 while reloading.
  Always run the normal application frame, preserving its loading/audio/render services.
- Restore the owned pause bit only after attachment and valid graphics; preserve
  unrelated mask bits. Reject swaps when a native pause already exists.
- Regression fixtures verify 324 delayed reloads progress application frames while
  gameplay tasks remain excluded. Live crash/freeze retesting is pending.

## 0.1.6 — protect the complete reload interval

- Replace the 0.1.4/0.1.5 actor action-bit guard. It did not protect command-menu
  cleanup, which also dereferences the temporarily missing weapon.
- Pause normal gameplay callbacks during the cosmetic reload. Progress only
  vanilla file/sound task groups and their dedicated I/O worker until weapon
  attachment and graphics initialization complete.
- Preserve the previous frame result while waiting; reject overlapping resource
  loads and never resume gameplay merely because a timeout expired.
- Native regressions cover 324 delayed reloads with missing weapons and incomplete
  graphics. No gameplay callback runs in either unsafe state.
- Both reported crash paths and seamless cycling still need live retesting.

## 0.1.5 — Q works after attacking

- Remove the mistaken weapon +0x41 readiness check. This persistent hit counter
  remains nonzero after a completed swing and is not an active-attack flag.
- Keep valid-weapon checks and the temporary action gate during asynchronous loads.
- Add native regressions for swapping with hit counters 1 and 255.

## 0.1.4 — asynchronous swap protection

- Accept Q only with a valid attached weapon, a finished loader, and no active
  hit or other Sora action state.
- Temporarily use Sora's native action gate while the replacement loads; restore
  that flag only after a valid weapon is attached. Repeated Q cannot overlap loads.
- Reject empty loader state 1, which earlier versions accepted as ready.
- Regression tests cover empty hands during loading, timeout protection, action
  flag restoration, and actor replacement. Live crash reproduction still needs retesting.

## 0.1.3 — cosmetic hit sounds

- Selected appearance now supplies both model and hit sounds: load its sound
  bank and copy only its four-byte hit-sound base ID alongside the model name.
- Preserve all combat fields, including damage, MP, reach, and critical behavior.
- Shift+Q restores the equipped weapon's original model and hit sounds.
- Remove the 0.1.2 sound-filename hooks; use vanilla model/sound loading directly.
- Native Windows regression tests cover all 324 equipped/appearance pairs,
  exact reset, stat-only mods, and competing cosmetic edits.
- Audio verification after a normal game restart is still pending.

## 0.1.2 — equipped hit-sound preservation

- Keep original equipped Keyblade sound-bank names in both vanilla sound-loading
  paths, while continuing to load the selected cosmetic weapon model.
- Preserve every sound ID, combat parameter, and equipment ID.
- Room-transition appearance persistence confirmed by gameplay feedback.
- Sound fix requires a full game restart; in-game audio verification pending.

## 0.1.1 — item-table fix

- Correct item metadata lookup from 0x2D22D30 to 0x2D22D38; valid equipped
  Keyblades are no longer rejected by the swap guard.
- Added regression check against the supported executable's actual item getter
  and all 18 real item metadata records.

## 0.1.0 — experimental preview

- Q cycles 18 standard Final Mix Keyblade appearances.
- Shift+Q restores the currently equipped Keyblade's original appearance.
- Equipped item and combat parameters remain the stat source.
- Native keyboard polling and model refresh run through the game frame callback.
- Preview pending live gameplay verification.
