-- Keyblade Transmog preview 0.1.2; Steam Global 1.0.0.2 / LuaBackend.
-- Native helper runs input and asynchronous model refresh on game's frame thread.
LUAGUI_NAME = "Keyblade Transmog (Q)"
LUAGUI_AUTH = "ROXASBrandon"
LUAGUI_DESC = "Q cycles cosmetic Keyblade models; equipped stats stay unchanged. Shift+Q resets."
local bootstrap
function _OnInit()
    bootstrap = nil
    if GAME_ID ~= 0xAF71841E or ENGINE_TYPE ~= "BACKEND" then
        ConsolePrint("Keyblade Transmog: KH1 LuaBackend required; disabled.")
        return
    end
    if ReadByte(0x4698D2) ~= 106 or ReadInt(0x3EA388) ~= 540680280 then
        ConsolePrint("Keyblade Transmog: unsupported game version; disabled.")
        return
    end
    local path = SCRIPT_PATH .. "/io_packages/kh1_transmog.dll"
    local loader, reason = package.loadlib(path, "kh1_transmog_bootstrap")
    if not loader then
        ConsolePrint("Keyblade Transmog: helper missing/unloadable; disabled. " .. tostring(reason))
        return
    end
    bootstrap = loader
end
function _OnFrame()
    if bootstrap then bootstrap() end
end
