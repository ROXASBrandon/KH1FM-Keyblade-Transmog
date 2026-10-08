# Keyblade Transmog

![Sora cycling Keyblade appearances in gameplay](images/banner.gif)

[Download on Nexus Mods](https://www.nexusmods.com/kingdomheartsfinalmix/mods/260) · [GitHub Release](https://github.com/ROXASBrandon/KH1FM-Keyblade-Transmog/releases/latest) · [Report a bug](https://github.com/ROXASBrandon/KH1FM-Keyblade-Transmog/issues)

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

## Installation methods

Choose **one** method below. Close Kingdom Hearts normally before installing or updating, and keep only one copy of this mod enabled.

### OpenKH Mods Manager — GitHub

1. Open **OpenKH Mods Manager** and select **Kingdom Hearts 1**.
2. Open **Mods > Install new mods** or click **+**.
3. Enter `ROXASBrandon/KH1FM-Keyblade-Transmog` in the GitHub field.
4. Click **Install**, then enable **Keyblade Transmog (Q)**.
5. Click **Mod Loader > Build and Run**.

### OpenKH Mods Manager — downloaded ZIP

1. Download the mod ZIP from the **Files** tab on [Nexus Mods](https://www.nexusmods.com/kingdomheartsfinalmix/mods/260) or from [GitHub Releases](https://github.com/ROXASBrandon/KH1FM-Keyblade-Transmog/releases/latest).
2. Open **OpenKH Mods Manager** and select **Kingdom Hearts 1**.
3. Open **Mods > Install new mods** or click **+**.
4. Choose **Select and install Mod Archive or Lua Script**, then select the downloaded ZIP **without extracting it**.
5. Enable **Keyblade Transmog (Q)**, then click **Mod Loader > Build and Run**.

Load gameplay and stay in the same area briefly while appearances preload. Once ready, tap **Q** to switch.

### Updating

For a GitHub installation, close the game, use **Settings > Check Mods for Updates**, then rebuild and restart. For a ZIP installation, close the game, remove the previous imported copy, import the new ZIP, and rebuild.

Restart after updating; reloading scripts alone cannot replace the native helper.

[OpenKH installation guide](https://github.com/OpenKH/OpenKh/blob/master/docs/tool/GUI.ModsManager/index.md#installing-mods).
## Notes

Selection lasts for the current game session and is not stored in your save. Restarting restores the equipped appearance. Holding Q does not repeatedly cycle. Switching waits for normal gameplay; menu, cutscene, death, and transition presses are ignored. Rebind any game action that also uses Q to avoid triggering both.

Dream weapons, Wooden Sword, and special story equipment are excluded. Scripted cutscene props may use their own models. Other Keyblade replacement mods are unverified and may prevent switching.

Tested independently and alongside Keyblade of Heart through OpenKH Build and Run.

## Troubleshooting Q and console status

Press F2 to open LuaBackend's console. In 0.2.0-p9, successful helper loading
prints `Transmog: helper loaded`, followed by native hook and preload messages.
Wait for `Ready experimental 0.2.0-p9` before testing Q. Stay in the same area
while the initial 36 loads complete. If preload fails or the scene changes during
preload, close and restart KH1 to retry.

LuaBackend's generic “Initialization successful” message only confirms that the
Lua script loaded. It does not confirm that the native helper is ready. Versions
before p9 may have working cosmetic switching without visible native messages.
Report unsupported-build, helper-unavailable, hook, or preload errors together
with your game version/language and enabled mods. Install the Lua script and DLL
from the same package; F1 cannot replace the pinned native helper.

## Removal

Close the game, disable this mod in OpenKH, rebuild your enabled mods, and restart.

## Source and credits

Build instructions and validation details are in [development notes](DEVELOPMENT.md). See [credits](CREDITS.md) and [changelog](CHANGELOG.md).

## My other mods

- [Keyblade of Heart](https://github.com/ROXASBrandon/KH1FM-Keyblade-of-Heart) — Riku's blade, dark blue trail, and swing sounds over Kingdom Key.
- [Treasure Magnet Starter](https://github.com/ROXASBrandon/KH1FM-Treasure-Magnet-Starter) — early unlock, zero AP.
- [Treasure Magnet Vacuum](https://github.com/ROXASBrandon/KH1FM-Treasure-Magnet-Vacuum) — expanded pickup range for items and HP/MP/munny orbs.
