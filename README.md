# Keyblade Transmog

![Sora cycling Keyblade appearances in gameplay](images/banner.gif)

[Download](https://github.com/ROXASBrandon/KH1FM-Keyblade-Transmog/releases/latest) · [Report a bug](https://github.com/ROXASBrandon/KH1FM-Keyblade-Transmog/issues)

Press **Q** to instantly cycle through **18 Keyblade appearances**, with matching trails and hit sounds, while keeping your equipped weapon's stats and abilities. **Shift+Q** restores its original look.

Compatible with my [Keyblade of Heart mod](https://github.com/ROXASBrandon/KH1FM-Keyblade-of-Heart).

## Features

- Switch appearances during normal gameplay without re-equipping your weapon.
- Keep your equipped weapon's stats, abilities, and combat properties.
- Use matching cosmetic trails and weapon hit sounds.
- Keep your selected appearance through area changes and death/retry during the current game session.
- Restore your equipped weapon's appearance with **Shift+Q**.

Equip Jungle King and display Oathkeeper: Sora looks and sounds like Oathkeeper, while Jungle King supplies the combat stats. Switching does not grant weapons or change your inventory.

## Controls and appearances

| Control | Action |
| --- | --- |
| **Q** | Cycle to the next appearance |
| **Shift+Q** | Restore the equipped weapon's appearance |

The first Q selects Kingdom Key, then cycles through Jungle King, Three Wishes, Fairy Harp, Pumpkinhead, Crabclaw, Divine Rose, Spellbinder, Olympia, Lionheart, Metal Chocobo, Oathkeeper, Oblivion, Lady Luck, Wishing Star, Ultima Weapon, Diamond Dust, and One-Winged Angel.

With Keyblade of Heart enabled, the Kingdom Key appearance displays that replacement. It does not add another cycle slot.

## Requirements

Kingdom Hearts Final Mix on Steam, LuaBackend, and OpenKH Mods Manager with Panacea. See [development notes](DEVELOPMENT.md) for supported executable details.

## Installation

1. Download the mod ZIP from [Releases](https://github.com/ROXASBrandon/KH1FM-Keyblade-Transmog/releases/latest).
2. Close Kingdom Hearts normally.
3. Select **Kingdom Hearts 1** in OpenKH Mods Manager.
4. Open **Mods > Install new mods** or click **+**, choose **Select and install Mod Archive or Lua Script**, and select the downloaded ZIP without extracting it.
5. Enable **Keyblade Transmog (Q)**, then click **Build and Run**.
6. Load gameplay and stay in the same area briefly while the appearances preload. Once ready, tap **Q** to switch.

For updates, close the game, remove the previous imported copy, import the new ZIP, and rebuild. Keep one copy enabled. Restart the game after updating; script reload alone cannot replace the native helper.

## Notes

Selection lasts for the current game session and is not stored in your save. Restarting restores the equipped appearance. Holding Q does not repeatedly cycle. Switching waits for normal gameplay; menu, cutscene, death, and transition presses are ignored. Rebind any game action that also uses Q to avoid triggering both.

Dream weapons, Wooden Sword, and special story equipment are excluded. Scripted cutscene props may use their own models. Other Keyblade replacement mods are unverified and may prevent switching.

Tested independently and alongside Keyblade of Heart through OpenKH Build and Run.

## Removal

Close the game, disable this mod in OpenKH, rebuild your enabled mods, and restart.

## Source and credits

Build instructions and validation details are in [development notes](DEVELOPMENT.md). See [credits](CREDITS.md) and [changelog](CHANGELOG.md).

## My other mods

- [Keyblade of Heart](https://github.com/ROXASBrandon/KH1FM-Keyblade-of-Heart) — Riku's blade, dark blue trail, and swing sounds over Kingdom Key.
- [Treasure Magnet Starter](https://github.com/ROXASBrandon/KH1FM-Treasure-Magnet-Starter) — early unlock, zero AP.
- [Treasure Magnet Vacuum](https://github.com/ROXASBrandon/KH1FM-Treasure-Magnet-Vacuum) — expanded pickup range for items and HP/MP/munny orbs.
