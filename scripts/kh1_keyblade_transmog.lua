-- Experimental 0.2.0-p9: all 18 Keyblade graphics/effects/sound prototype.
LUAGUI_NAME = "Keyblade Transmog - Seamless Prototype"
LUAGUI_AUTH = "ROXASBrandon"
LUAGUI_DESC = "Q toggles 18 preloaded looks, new trails and hit sounds; Shift+Q resets. Retains look across areas and death/retry."
local bootstrap
function _OnInit()
    bootstrap = nil
    if GAME_ID ~= 0xAF71841E or ENGINE_TYPE ~= "BACKEND" then return end
    if ReadByte(0x4698D2) ~= 106 or ReadInt(0x3EA388) ~= 540680280 then
        ConsolePrint("Seamless prototype: unsupported game build.")
        return
    end
    local loader, reason = package.loadlib(SCRIPT_PATH .. "/io_packages/kh1_transmog.dll", "kh1_transmog_bootstrap")
    if not loader then
        ConsolePrint("Seamless prototype: helper unavailable: " .. tostring(reason))
        return
    end
    bootstrap = loader
    ConsolePrint("Transmog: helper loaded. Stay in the same area until native preload reports Ready.")
end
function _OnFrame()
    if bootstrap then bootstrap() end
end
