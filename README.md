# Keyblade Transmog

![Sora seamlessly cycling Keyblades — Keyblade Transmog animated gameplay banner](images/banner.gif)

[Download p8 compatibility preview](downloads/Keyblade-Transmog-v0.2.0-p8-preview.zip) · [Report a bug](https://github.com/ROXASBrandon/KH1FM-Keyblade-Transmog/issues) · [GitHub Release](https://github.com/ROXASBrandon/KH1FM-Keyblade-Transmog/releases/tag/v0.2.0-p7)

**Version 0.2.0-p8 compatibility preview — ROXASBrandon**

Supports the exact Keyblade of Heart v0.1.5 replacement alongside vanilla weapons.
The release link above points to the published p7 base; the p8 ZIP is included in this update. The author confirmed p8 and Heart v0.1.5 work together in gameplay.

Press **Q** to instantly cycle all 18 standard Final Mix Keyblade appearances,
with matching new trails and weapon hit sounds. **Shift+Q** restores the equipped
blade's look, trails, and sounds. Your equipped weapon keeps its stats and abilities.

This seamless preview passed the author's gameplay checks: all 18 appearances,
battle switching and repeated Q presses, equipped stats/abilities, matching trails
and hit sounds, chests, area transitions, death/retry, and Shift+Q reset. Those gameplay checks apply to the p7 base. p8 changes only asset compatibility
validation; private-memory regression tests pass, and the author confirmed both mods work together in gameplay. Other PCs and OpenKH installation remain
unverified.

Requires **Kingdom Hearts Final Mix, Steam Global/WW 1.0.0.2**, and **LuaBackend**.

## How it works

After loading gameplay, the mod preloads 18 cosmetic model/effect banks and 18
sound banks through the game's normal loader. Once ready, Q switches the submitted
weapon graphics and new cosmetic effects/sounds without detaching or reloading
your actual equipped weapon. Switching needs no gameplay pause and works during
swings. Existing trails and sounds finish naturally.

Equip Jungle King and show Oathkeeper: Sora looks and sounds like Oathkeeper,
while Jungle King supplies strength, MP, critical behavior, reach, and other weapon
parameters. Normal equipment changes change the stat source. Cosmetic switching
does not grant weapons or modify your inventory.

The first Q selects **Kingdom Key**, then cycles through:
Jungle King, Three Wishes, Fairy Harp, Pumpkinhead, Crabclaw, Divine Rose,
Spellbinder, Olympia, Lionheart, Metal Chocobo, Oathkeeper, Oblivion, Lady Luck,
Wishing Star, Ultima Weapon, Diamond Dust, One-Winged Angel, then Kingdom Key.
After Shift+Q, the next Q starts at Kingdom Key again. Holding Q does not cycle.

The selection survives area changes and death/retry during the same game session.
Visible standard blades keep their cosmetic appearance through chest animations
and area fade-in. Native hidden blades remain hidden; separately loaded cutscene
props may use their scripted models. Selection is not written to your save;
restarting the game restores the equipped appearance.

## Install or update

**Close KH1 normally before replacing the Lua script or native DLL.** Install both
files together. F1 script reload cannot replace the pinned native helper.

Choose **one** method:

1. **OpenKH / GitHub:** select Kingdom Hearts 1 in Mods Manager, open **Mods >
   Install new mods**, and enter `ROXASBrandon/KH1FM-Keyblade-Transmog` in the GitHub field.
   Click **Install**, enable the mod, then **Mod Loader > Build and Run**.
   For updates, use **Settings > Check Mods for Updates** and rebuild after closing KH1.
2. **Manual LuaBackend:** copy `scripts/kh1_keyblade_transmog.lua` to your KH1
   script folder, and `scripts/io_packages/kh1_transmog.dll` to its `io_packages`
   subfolder. Replace the previous Transmog pair and keep only one copy enabled.
3. **OpenKH / downloaded ZIP:** [download the p8 ZIP](downloads/Keyblade-Transmog-v0.2.0-p8-preview.zip).
   Select Kingdom Hearts 1, open **Mods > Install new mods** (or the **+** button),
   click **Select and install Mod Archive or Lua Script**, and select the downloaded
   ZIP **without extracting it**. Enable the imported mod, then **Mod Loader >
   Build and Run**. The ZIP contains `mod.yml` at its root and all required files.
   For later ZIP updates, close KH1, remove the previous imported copy from Mods
   Manager, import the new ZIP, and rebuild. Keep only one copy of this mod enabled.

[OpenKH's official archive installation guide](https://github.com/OpenKH/OpenKh/blob/master/docs/tool/GUI.ModsManager/index.md#installing-mods).

Use one install method. Restart KH1, load gameplay, and stand still briefly to
start preloading. **Stay in the same area until ready.** F2 opens LuaBackend's
console; progress shows `Preload 1/36` through `36/36`, then
`[Seamless Prototype] Ready experimental 0.2.0-p8`.

After readiness, tap Q to cycle or Shift+Q to reset. Early, unfocused, menu,
cutscene, death, and transition presses are ignored rather than queued. Q input,
new trails, and hit sounds resume after gameplay settles; display can retain the
chosen look before the HUD returns. If Q also has a native game binding, rebind
that action in the game's settings to avoid triggering both.

## Compatibility and limits

- Only the supported Steam Global/WW 1.0.0.2 executable and vanilla weapon asset
  layouts are supported, plus the exact Heart v0.1.5 weapon replacement in p8.
  Other replacement mods may fail validation and prevent switching.
- Dream weapons, Wooden Sword, special story equipment, and gummi are excluded.
  Native visibility and scripted props remain under the game's control.
- Initial preloading makes at most 36 native file requests per process. A scene
  change during preloading, malformed asset, or failed preload disables cosmetics
  until restart. Q does not issue further loads.
- Borrowed native resources are validated before drawing. If they unload,
  overrides disable until restart. Missing individual audio clips use equipped
  sounds. The helper does not free resources still owned by the game.
- No game assets, game executables, saves, accounts, or personal logs are bundled.
  The mod does not edit saves or game archives.

Automated tests cover 4,096 render substitutions with unchanged weapon bytes and
pause state, all 18 appearances, 630 hit variants, all trail banks, native hook
ABI, held/busy input, chest rendering, transition fade-in, actor replacement,
death/retry with cleared sound bookkeeping, and unloaded-resource fallbacks.
See [development notes](DEVELOPMENT.md) and [changelog](CHANGELOG.md).

## Remove or roll back

Close KH1. Remove this script and its DLL, or disable the OpenKH mod and rebuild.
Restart. Removing a script or reloading scripts alone does not unload the native
hooks. Keep a backup of both files before updating.

The [previous 0.1.12 preview](https://github.com/ROXASBrandon/KH1FM-Keyblade-Transmog/raw/refs/heads/main/downloads/Keyblade-Transmog-v0.1.12-preview.zip)
remains available for rollback. Restore both files from that package with KH1
closed, then restart. Its swaps require idle gameplay and briefly pause to reload.

## Build from source

Install Python 3 and `ziglang==0.16.0`, then run `python native/build.py`.
Windows also runs the private-memory native regression tests; Linux/WSL
cross-compiles the Windows x64 DLL. Building does not install anything.
Run `python tools/package.py` to build and create the ZIP and SHA256 checksums.
`--skip-build` packages the existing DLL, as used for this exact tested release.

On Linux, cross-compile `tests/native_test.c` with the `native` include directory
and execute it on Windows. Optional read-only checks use Python `pefile` and
`python tests/verify_executable.py <game-executable> <local-vanilla-asset-directory>`.
Supported header metadata is checked in; full game assets are not required to
build. `native/generate_headers.py` regenerates that metadata from a local extraction.

## My other mods

- [Treasure Magnet Starter](https://github.com/ROXASBrandon/KH1FM-Treasure-Magnet-Starter) — early unlock, zero AP.
- [Treasure Magnet Vacuum](https://github.com/ROXASBrandon/KH1FM-Treasure-Magnet-Vacuum) — 500x pickup range for items and HP/MP/munny orbs.

See [credits](CREDITS.md).

## Keyblade of Heart compatibility

p8 accepts vanilla weapon headers as before. Kingdom Key additionally accepts
the exact Heart v0.1.5 asset through full SHA-256 verification before native
initialization, plus expected headers, bounds, effect layout, and resource checks.
Other Keyblade model replacements may fail preload and disable switching.
Only vanilla assets and this exact Heart version have supported profiles. With Heart enabled, the
Kingdom Key appearance slot displays Heart and its effects; no new cycle slot is
added. Stats and inventory stay tied to the equipped weapon.

Windows source tests require the Heart payload fixture. Build with:

```text
python native/build.py PATH_TO_HEART_MOD/generated/xw_ex_5010.wpn
python tools/package.py --skip-build
```

Companion: **Keyblade of Heart v0.1.5**. Its public mod page is being prepared;
the two pages will link to each other before public release. Updating its weapon
asset requires an updated compatibility fingerprint in Transmog.
