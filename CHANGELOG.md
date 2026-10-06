# Changelog

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
