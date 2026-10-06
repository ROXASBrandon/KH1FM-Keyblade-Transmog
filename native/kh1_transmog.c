/* KH1 Steam Global 1.0.0.2. Built locally; no Lua C API dependency. */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include "transmog_core.h"

static uintptr_t game;
static int enabled, failed, captured, selected = -1, was_q;
static unsigned char originals[TM_COUNT][TM_MODEL_SIZE], applied[TM_MODEL_SIZE];
static uintptr_t table;
static ULONGLONG last_swap;
typedef uint64_t (__cdecl *FrameProc)(void *);
static FrameProc original_frame;
typedef void (__fastcall *EquipProc)(int, void *);
typedef int (__cdecl *FormatProc)(char *, const char *, const char *, const char *);

static int __cdecl format_sound_name(char *dest, const char *format,
                                    const char *model, const char *extension) {
    return ((FormatProc)(game+0x51260))(dest,format,
        tm_sound_model(model,(const unsigned char *)table,originals,captured),extension);
}
static int install_sound_hooks(void) {
    /* These two calls format .se filenames: direct re-equip and room loading.
       Leave the later .wpn formatter and every sound-ID/stat field untouched. */
    const uintptr_t sites[]={game+0x286CCC,game+0x2871DA};
    unsigned char *relay=NULL;
    uintptr_t cursor=game,limit=game+0x70000000;
    while (cursor<limit) {
        MEMORY_BASIC_INFORMATION m;
        if (!VirtualQuery((void *)cursor,&m,sizeof(m))) break;
        uintptr_t end=(uintptr_t)m.BaseAddress+m.RegionSize;
        if (end<=cursor) break;
        uintptr_t candidate=(cursor+0xFFFF)&~(uintptr_t)0xFFFF;
        if (m.State==MEM_FREE && candidate<limit && candidate<=end && end-candidate>=0x1000) {
            relay=VirtualAlloc((void *)candidate,0x1000,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);
            if (relay) break;
        }
        cursor=end;
    }
    if (!relay) return 0;
    /* mov rax, function; jmp rax. CALL at each site retains its return address. */
    relay[0]=0x48;relay[1]=0xB8;
    uintptr_t target=(uintptr_t)&format_sound_name;
    memcpy(relay+2,&target,sizeof(target));relay[10]=0xFF;relay[11]=0xE0;
    DWORD relay_protection,protection[2];
    if (!VirtualProtect(relay,0x1000,PAGE_EXECUTE_READ,&relay_protection)) {
        VirtualFree(relay,0,MEM_RELEASE);return 0;
    }
    FlushInstructionCache(GetCurrentProcess(),relay,12);
    for (int i=0;i<2;++i) {
        if (!VirtualProtect((void *)sites[i],5,PAGE_EXECUTE_READWRITE,&protection[i])) {
            for (int j=0;j<i;++j) {DWORD ignored;VirtualProtect((void *)sites[j],5,protection[j],&ignored);}
            VirtualFree(relay,0,MEM_RELEASE);return 0;
        }
    }
    for (int i=0;i<2;++i) {
        int32_t displacement=(int32_t)((intptr_t)relay-(intptr_t)(sites[i]+5));
        memcpy((void *)(sites[i]+1),&displacement,4);
        FlushInstructionCache(GetCurrentProcess(),(void *)sites[i],5);
    }
    for (int i=1;i>=0;--i) {DWORD ignored;VirtualProtect((void *)sites[i],5,protection[i],&ignored);}
    return 1;
}

static void log_message(const char *text) {
    printf("[Keyblade Transmog] %s\n", text); fflush(stdout);
}
static int readable(uintptr_t addr, size_t size) {
    MEMORY_BASIC_INFORMATION m;
    if (!addr || !VirtualQuery((void *)addr, &m, sizeof(m))) return 0;
    return m.State == MEM_COMMIT && !(m.Protect & (PAGE_GUARD | PAGE_NOACCESS))
        && addr + size >= addr && addr + size <= (uintptr_t)m.BaseAddress + m.RegionSize;
}
static int bytes_at(uintptr_t rva, const unsigned char *bytes, size_t count) {
    return readable(game + rva, count) && memcmp((void *)(game + rva), bytes, count) == 0;
}
static int signature_ok(void) {
    static const unsigned char frame_sig[] = {0x48,0x89,0x35,0xFF,0x44,0x0D,0x02,0x48,0x8B,0xC6};
    static const unsigned char equip_sig[] = {0x48,0x89,0x5C,0x24,0x18,0x55,0x56,0x41,0x56,0x48,0x83,0xEC,0x20,0x4C,0x8B,0xF2};
    static const unsigned char item_sig[] = {0x8D,0x41,0xFF,0x48,0x63,0xC8,0x48,0x8B,0x05,0xBB,0x33,0xA9,0x02};
    static const unsigned char sound_equip_sig[] = {0xE8,0x8F,0xA5,0xDC,0xFF};
    static const unsigned char sound_room_sig[] = {0xE8,0x81,0xA0,0xDC,0xFF};
    return *(unsigned char *)(game+0x4698D2)==106
        && *(uint32_t *)(game+0x3EA388)==540680280
        && bytes_at(0xD6A12,frame_sig,sizeof(frame_sig))
        && bytes_at(0x286720,equip_sig,sizeof(equip_sig))
        && bytes_at(0x28F970,item_sig,sizeof(item_sig))
        && bytes_at(0x286CCC,sound_equip_sig,sizeof(sound_equip_sig))
        && bytes_at(0x2871DA,sound_room_sig,sizeof(sound_room_sig));
}
static int capture_table(void) {
    uintptr_t save = *(uintptr_t *)(game + 0x2868BA0);
    if (!readable(save,0x74)) return 0;
    /* Lookup uses btltbl base + offset from character table at 0x528060. */
    table = game + 0x2D22D40 + *(uint32_t *)(game + 0x528060);
    if (!readable(table,22*TM_STRIDE)) return 0;
    for (int i=0;i<TM_COUNT;++i) {
        unsigned char *record=(unsigned char *)(table+tm_rows[i]*TM_STRIDE);
        if (memcmp(record,tm_models[i],11)) return 0;
    }
    for (int i=0;i<TM_COUNT;++i)
        memcpy(originals[i],(void *)(table+tm_rows[i]*TM_STRIDE),TM_MODEL_SIZE);
    captured=1;
    log_message("Ready. Q cycles 18 Keyblade looks; Shift+Q restores equipped look. Combat stats stay equipped.");
    return 1;
}
static void restore_records(void) {
    for (int i=0;i<TM_COUNT;++i) {
        unsigned char *record=(unsigned char *)(table+tm_rows[i]*TM_STRIDE);
        if (readable((uintptr_t)record,TM_MODEL_SIZE) && !memcmp(record,applied,TM_MODEL_SIZE))
            tm_copy_model(record,originals[i]);
    }
}
static int equipped_row(int *item_out) {
    uintptr_t save=*(uintptr_t *)(game+0x2868BA0);
    uintptr_t items=*(uintptr_t *)(game+0x2D22D38);
    if (!readable(save,0x74) || !readable(items,0x900)) return -1;
    int item=*(unsigned char *)(save+0x36);
    if (!(item==81 || (item>=86 && item<=102))) return -1;
    int row=*(int16_t *)(items+(item-1)*20+6)-1;
    if (tm_index_for_row(row)<0) return -1;
    *item_out=item;
    return row;
}
static int gameplay_ready(void) {
    uintptr_t actor=*(uintptr_t *)(game+0x2D37280);
    return readable(actor,0x4B0) && (*(uint32_t *)(actor+0x374)&3)==1
        && *(unsigned char *)(game+0x2D5CC4C)>0
        && *(uint32_t *)(game+0x232DFA0)==0
        && *(unsigned char *)(game+0x22EC0AC)==0
        && *(unsigned char *)(game+0x23AB2D0)==0
        && *(uint32_t *)(game+0x5075A8)==0
        && *(float *)(game+0x281249C)>0.5f;
}
static int loader_ready(void) {
    uintptr_t slot=game+0x296B540; /* Sora owns first slot; companions use next two. */
    uint32_t flags=*(uint32_t *)slot;
    return (flags&1) && !(flags&2) && *(int *)(slot+4)==0
        && *(uintptr_t *)(game+0x296B5D0)==0;
}
static void apply_model(void) {
    restore_records();
    if (selected<0) return;
    for (int i=0;i<TM_COUNT;++i) {
        unsigned char *dest=(unsigned char *)(table+tm_rows[i]*TM_STRIDE);
        if (memcmp(dest,originals[i],TM_MODEL_SIZE)) {
            selected=-1;
            log_message("Weapon model table changed by another mod; cosmetic override stopped.");
            return;
        }
    }
    memcpy(applied,originals[selected],TM_MODEL_SIZE);
    for (int i=0;i<TM_COUNT;++i)
        tm_copy_model((unsigned char *)(table+tm_rows[i]*TM_STRIDE),applied);
}
static void tick(void) {
    int down=(GetAsyncKeyState('Q')&0x8000)!=0;
    DWORD foreground_pid=0;
    GetWindowThreadProcessId(GetForegroundWindow(),&foreground_pid);
    int focused=foreground_pid==GetCurrentProcessId();
    if (!captured && !capture_table()) { was_q=down; return; }
    int item=0,row=equipped_row(&item);
    int safe=focused && row>=0 && gameplay_ready();
    int edge=tm_key_edge(down,safe,&was_q);
    if (row<0) return;
    if (!safe) return;
    if (!edge || GetTickCount64()-last_swap<200 || !loader_ready()) return;
    int reset=(GetAsyncKeyState(VK_SHIFT)&0x8000)!=0;
    selected=reset ? -1 : tm_next(selected,row);
    apply_model();
    /* The vanilla equip routine re-equips the SAME item, releases old graphics,
       queues its .wpn, fixes up the loaded model and reattaches asynchronously.
       Invalidate the graphics cache only, forcing the filename to be reloaded. */
    *(int *)(game+0x296B548)=-1;
    ((EquipProc)(game+0x286720))(item,NULL);
    last_swap=GetTickCount64();
    char text[160];
    snprintf(text,sizeof(text),"Appearance: %s. Equipped item ID stays %d.",
        selected<0?"Original":tm_names[selected],item);
    log_message(text);
}
static uint64_t __cdecl on_frame(void *context) {
    tick();
    return original_frame(context);
}
/* Invoked via package.loadlib; returns zero Lua values, so no Lua ABI symbols
   are required even when LuaBackend statically links its Lua interpreter. */
__declspec(dllexport) int __cdecl kh1_transmog_bootstrap(void *lua_state) {
    (void)lua_state;
    if (enabled || failed) return 0;
    game=(uintptr_t)GetModuleHandleW(NULL);
    if (!signature_ok()) {failed=1;log_message("Unsupported executable/signature; disabled. Steam Global 1.0.0.2 required.");return 0;}
    uintptr_t app=*(uintptr_t *)(game+0x21AAF18);
    if (!readable(app,sizeof(uintptr_t))) return 0;
    uintptr_t vtable=*(uintptr_t *)app;
    if (!readable(vtable,5*sizeof(uintptr_t))) return 0;
    FrameProc *entry=(FrameProc *)(vtable+4*sizeof(uintptr_t));
    if (!*entry) return 0;
    DWORD protection;
    if (!VirtualProtect(entry,sizeof(*entry),PAGE_READWRITE,&protection)) {
        failed=1;log_message("Cannot install game frame hook; disabled.");return 0;
    }
    /* Pin until process exit: hot-reloading Lua cannot unload an active hook. */
    HMODULE pinned;
    if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_PIN,
        (LPCWSTR)(uintptr_t)&on_frame,&pinned)) {
        VirtualProtect(entry,sizeof(*entry),protection,&protection);
        failed=1;log_message("Cannot pin helper; disabled.");return 0;
    }
    if (!install_sound_hooks()) {
        VirtualProtect(entry,sizeof(*entry),protection,&protection);
        failed=1;log_message("Cannot install sound-name hooks; disabled.");return 0;
    }
    original_frame=(FrameProc)InterlockedExchangePointer((PVOID volatile *)entry,(PVOID)&on_frame);
    VirtualProtect(entry,sizeof(*entry),protection,&protection);
    enabled=1;
    log_message("Frame hook installed. Waiting for weapon table.");
    return 0;
}
