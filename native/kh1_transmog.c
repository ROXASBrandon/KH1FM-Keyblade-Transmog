/* KH1 Steam Global 1.0.0.2. Built locally; no Lua C API dependency. */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include "transmog_core.h"

static uintptr_t game;
static int enabled, failed, captured, selected = -1, was_q;
static unsigned char originals[TM_COUNT][TM_COSMETIC_SIZE], applied[TM_COSMETIC_SIZE];
static uintptr_t table;
static ULONGLONG last_swap;
static uintptr_t swap_actor;
static uint32_t swap_actor_model;
static int swap_pending, swap_warned;
/* Read-only diagnostics for verifying the installed hook during gameplay. */
__declspec(dllexport) volatile uint32_t kh1_transmog_build=112;
__declspec(dllexport) volatile uint32_t kh1_transmog_swaps;
__declspec(dllexport) volatile uint32_t kh1_transmog_paused_schedulers;
typedef void *(__fastcall *ResolveProc)(uint32_t);
static ResolveProc resolve_handle;
#define GAMEPLAY_PAUSE_MASK 0x2867370
typedef void (__fastcall *SchedulerProc)(void *, uint32_t);
static SchedulerProc original_scheduler;
#define SCHEDULER_CALL 0x2846B9
static void *scheduler_relay;
__declspec(dllexport) volatile uint32_t kh1_transmog_render_allowances;
#define CAMERA_SUBMIT_TASK 0x28CC10
#define MODEL_SUBMIT_TASK 0x2A23E0
#define ROOM_SUBMIT_TASK 0xD2830
#define AUDIO_QUEUE_TASK 0x290A50
__declspec(dllexport) volatile uint32_t kh1_transmog_audio_allowances;
static const unsigned char scheduler_call_original[5]={0xE8,0x52,0x60,0,0};
static uintptr_t idle_actor;
static uint64_t idle_since;
static int idle_tracking;
typedef uint64_t (__cdecl *FrameProc)(void *);
static FrameProc original_frame;
typedef void (__fastcall *EquipProc)(int, void *);
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
    static const unsigned char sound_sig[] = {0x48,0x8B,0x48,0x20,0x8B,0x49,0x34,0x03,0xCE};
    static const unsigned char action_sig[] = {0xF6,0x01,0x04,0x75,0x0F};
    static const unsigned char pause_sig[] = {0x8B,0x15,0xBF,0x2C,0x5E,0x02};
    static const unsigned char filter_sig[] = {0x0F,0xB7,0x47,0x02,0x41,0x23,0xC4,0x41,0x3B,0xC4};
    static const unsigned char resolve_sig[] = {0x85,0xC9,0x75,0x03,0x33,0xC0,0xC3};
    static const unsigned char camera_sig[]={0x48,0x83,0xEC,0x28,0x8B,0x15,0x46,0xA7,0x5D,0x02};
    static const unsigned char model_sig[]={0x48,0x83,0xEC,0x28,0x33,0xD2};
    static const unsigned char room_sig[]={0x48,0x83,0xEC,0x28,0x8B,0x05,0xB2,0xDC,0x26,0x02};
    static const unsigned char audio_sig[]={0x48,0x89,0x6C,0x24,0x10,0x48,0x89,0x74,0x24,0x18};
    static const unsigned char weapon_null_sig[]={0x48,0x85,0xC0,0x74,0x73};
    return *(unsigned char *)(game+0x4698D2)==106
        && *(uint32_t *)(game+0x3EA388)==540680280
        && bytes_at(0xD6A12,frame_sig,sizeof(frame_sig))
        && bytes_at(0x286720,equip_sig,sizeof(equip_sig))
        && bytes_at(0x28F970,item_sig,sizeof(item_sig))
        && bytes_at(0x2954E9,sound_sig,sizeof(sound_sig))
        && bytes_at(0x297830,action_sig,sizeof(action_sig))
        && bytes_at(0x38ADC0,resolve_sig,sizeof(resolve_sig))
        && bytes_at(0x2846AB,pause_sig,sizeof(pause_sig))
        && bytes_at(SCHEDULER_CALL,scheduler_call_original,5)
        && bytes_at(0x28A750,filter_sig,sizeof(filter_sig))
        && bytes_at(CAMERA_SUBMIT_TASK,camera_sig,sizeof(camera_sig))
        && bytes_at(MODEL_SUBMIT_TASK,model_sig,sizeof(model_sig))
        && bytes_at(ROOM_SUBMIT_TASK,room_sig,sizeof(room_sig))
        && bytes_at(AUDIO_QUEUE_TASK,audio_sig,sizeof(audio_sig))
        && bytes_at(0x2A17D8,weapon_null_sig,sizeof(weapon_null_sig));
}
/* Native pause handling rewrites GAMEPLAY_PAUSE_MASK inside the normal frame.
   Inject the reload mask at the final scheduler call, after those writes. */
static int render_task(uintptr_t callback) {
    return callback==game+CAMERA_SUBMIT_TASK || callback==game+MODEL_SUBMIT_TASK
        || callback==game+ROOM_SUBMIT_TASK;
}
static int reload_task(uintptr_t callback) {
    return render_task(callback) || callback==game+AUDIO_QUEUE_TASK;
}
static void __fastcall on_scheduler(void *group, uint32_t mask) {
    struct {uintptr_t node,callback;} owned[8];int count=0;
    if (swap_pending) {
        ++kh1_transmog_paused_schedulers;
        /* Keep native iteration order. Submit camera, room and models (the
           model weapon branch checks null), and drain native audio commands.
           Sound-bank install waits for that queue; excluding its updater
           deadlocks companion commands during battle. Attack, cleanup, AI
           and unknown tasks retain the reload pause mask. */
        uintptr_t node=readable((uintptr_t)group,0x18)?*(uintptr_t *)((uintptr_t)group+0x10):0;
        for (int visited=0;node && visited<256;++visited) {
            if (!readable(node,0x18)) break;
            uintptr_t callback=*(uintptr_t *)(node+0x10);
            uint16_t *flags=(uint16_t *)(node+2);
            if (reload_task(callback) && !(*flags&1) && count<8) {
                owned[count].node=node;owned[count].callback=callback;++count;
                *flags|=1;
                if (render_task(callback)) ++kh1_transmog_render_allowances;
                else ++kh1_transmog_audio_allowances;
            }
            node=*(uintptr_t *)(node+8);
        }
    }
    original_scheduler(group, mask | (swap_pending ? 1u : 0u));
    for (int i=0;i<count;++i) {
        uintptr_t node=owned[i].node;
        if (readable(node,0x18) && *(uintptr_t *)(node+0x10)==owned[i].callback)
            *(uint16_t *)(node+2)&=(uint16_t)~1u;
    }
}
static int install_scheduler_hook(void) {
    unsigned char *site=(unsigned char *)(game+SCHEDULER_CALL);
    if (!bytes_at(SCHEDULER_CALL,scheduler_call_original,5)) return 0;
    /* A rel32 call needs a relay within 2 GiB. Search free allocation-granularity
       addresses near the executable, then seal the relay as execute/read. */
    SYSTEM_INFO info;GetSystemInfo(&info);
    uintptr_t gran=info.dwAllocationGranularity;
    uintptr_t anchor=(uintptr_t)site&~(gran-1);
    for (uintptr_t distance=gran;distance<0x7FFF0000u && !scheduler_relay;distance+=gran) {
        uintptr_t candidates[2]={anchor+distance,anchor>=distance?anchor-distance:0};
        for (int i=0;i<2 && !scheduler_relay;++i) {
            uintptr_t candidate=candidates[i];
            if (!candidate || candidate<(uintptr_t)info.lpMinimumApplicationAddress
                || candidate>(uintptr_t)info.lpMaximumApplicationAddress) continue;
            MEMORY_BASIC_INFORMATION region;
            if (!VirtualQuery((void *)candidate,&region,sizeof(region)) || region.State!=MEM_FREE) continue;
            scheduler_relay=VirtualAlloc((void *)candidate,gran,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);
        }
    }
    if (!scheduler_relay) return 0;
    unsigned char relay[12]={0x48,0xB8};
    uintptr_t target=(uintptr_t)&on_scheduler;
    memcpy(relay+2,&target,8);relay[10]=0xFF;relay[11]=0xE0; /* mov rax,target; jmp rax */
    memcpy(scheduler_relay,relay,sizeof(relay));
    DWORD old;
    if (!VirtualProtect(scheduler_relay,gran,PAGE_EXECUTE_READ,&old)) goto fail;
    FlushInstructionCache(GetCurrentProcess(),scheduler_relay,sizeof(relay));
    int64_t relative=(int64_t)(uintptr_t)scheduler_relay-(int64_t)((uintptr_t)site+5);
    if (relative<INT32_MIN || relative>INT32_MAX) goto fail;
    unsigned char call[5]={0xE8};int32_t displacement=(int32_t)relative;
    memcpy(call+1,&displacement,4);
    if (!VirtualProtect(site,5,PAGE_EXECUTE_READWRITE,&old)) goto fail;
    original_scheduler=(SchedulerProc)(game+0x28A710);
    memcpy(site,call,5);FlushInstructionCache(GetCurrentProcess(),site,5);
    VirtualProtect(site,5,old,&old);
    return 1;
fail:
    VirtualFree(scheduler_relay,0,MEM_RELEASE);scheduler_relay=NULL;
    return 0;
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
        tm_capture_cosmetic(originals[i],(const unsigned char *)(table+tm_rows[i]*TM_STRIDE));
    captured=1;
    log_message("Ready 0.1.12. Q cycles 18 Keyblade looks and hit sounds; Shift+Q restores equipped look/sounds. Combat stats stay equipped.");
    return 1;
}
static void restore_records(void) {
    for (int i=0;i<TM_COUNT;++i) {
        unsigned char *record=(unsigned char *)(table+tm_rows[i]*TM_STRIDE);
        if (readable((uintptr_t)record,TM_SOUND_OFFSET+TM_SOUND_SIZE) && tm_cosmetic_matches(record,applied))
            tm_copy_cosmetic(record,originals[i]);
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
    /* State 5 is one attached owner. State 1 has no attached weapon; the old
       guard incorrectly accepted it as ready. State 3 is still loading. */
    return (flags&7)==5 && *(int *)(slot+4)==0
        && *(uintptr_t *)(game+0x296B5D0)==0;
}
static uintptr_t actor_weapon(uintptr_t actor) {
    if (!resolve_handle || !readable(actor,0x150)) return 0;
    uint32_t handle=*(uint32_t *)(actor+0x14C);
    if (!handle) return 0;
    uintptr_t weapon=(uintptr_t)resolve_handle(handle);
    return readable(weapon,0x60) ? weapon : 0;
}
static int attached_model_ready(uintptr_t actor) {
    uintptr_t weapon=actor_weapon(actor);
    return weapon && *(uint32_t *)weapon
        && readable(*(uintptr_t *)(weapon+0x18),1);
}
static int swap_ready(uintptr_t actor) {
    uintptr_t weapon=actor_weapon(actor);
    /* Use Sora's action gate/state. Weapon +0x41 is a persistent hit counter,
       not an attack-in-progress flag: rejecting it blocks Q after a swing. */
    return readable(actor,0x448) && !(*(unsigned char *)actor&4)
        && !(*(unsigned char *)(actor+0x88)&0x20)
        && !(*(unsigned char *)(actor+0x444)&3)
        && *(uint32_t *)(actor+0x70)==0
        && tm_animation_idle(*(uint32_t *)(actor+0x164)) && weapon
        && loader_ready() && attached_model_ready(actor)
        && *(uint32_t *)(game+GAMEPLAY_PAUSE_MASK)==0;
}
static void begin_swap(uintptr_t actor) {
    swap_actor=actor;
    swap_actor_model=*(uint32_t *)(actor+0x130);
    swap_pending=1;swap_warned=0;
    ++kh1_transmog_swaps;

}
static int finish_swap(void) {
    if (!swap_pending) return 1;
    /* A room/actor replacement must not unlock an unrelated actor. */
    uintptr_t actor=*(uintptr_t *)(game+0x2D37280);
    if (actor!=swap_actor || !readable(actor,0x150)
        || *(uint32_t *)(actor+0x130)!=swap_actor_model) {
        swap_pending=0;return 0;
    }
    if (loader_ready() && attached_model_ready(actor)) {
        swap_pending=0;
        return 1;
    }
    /* Filter gameplay tasks through the final scheduler argument, while keeping
       the application frame, audio, graphics and loading services running. */

    if (!swap_warned && GetTickCount64()-last_swap>3000) {
        swap_warned=1;
        log_message("Weapon reload stalled; gameplay remains paused to protect the missing weapon. Restart KH1 if it does not recover.");
    }
    return 0;
}
static void apply_model(void) {
    restore_records();
    if (selected<0) return;
    for (int i=0;i<TM_COUNT;++i) {
        unsigned char *dest=(unsigned char *)(table+tm_rows[i]*TM_STRIDE);
        if (!tm_cosmetic_matches(dest,originals[i])) {
            selected=-1;
            log_message("Weapon model/sound table changed by another mod; cosmetic override stopped.");
            return;
        }
    }
    memcpy(applied,originals[selected],TM_COSMETIC_SIZE);
    for (int i=0;i<TM_COUNT;++i)
        tm_copy_cosmetic((unsigned char *)(table+tm_rows[i]*TM_STRIDE),applied);
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
    uintptr_t actor=*(uintptr_t *)(game+0x2D37280);
    if (actor!=idle_actor) {idle_actor=actor;idle_tracking=0;}
    int settled=tm_idle_gate(safe && !swap_pending && swap_ready(actor),
        GetTickCount64(),&idle_since,&idle_tracking);
    if (!finish_swap()) return;
    if (row<0) return;
    if (!safe) return;
    if (!edge || !settled || GetTickCount64()-last_swap<200 || !swap_ready(actor)) return;
    int reset=(GetAsyncKeyState(VK_SHIFT)&0x8000)!=0;
    selected=reset ? -1 : tm_next(selected,row);
    apply_model();
    begin_swap(actor);
    idle_tracking=0;
    last_swap=GetTickCount64();
    /* The vanilla equip routine re-equips the SAME item, releases old graphics,
       queues its .wpn, fixes up the loaded model and reattaches asynchronously.
       Invalidate the graphics cache only, forcing the filename to be reloaded. */
    *(int *)(game+0x296B548)=-1;
    ((EquipProc)(game+0x286720))(item,NULL);
    char text[160];
    snprintf(text,sizeof(text),"Appearance/hit sounds: %s. Equipped item ID stays %d.",
        selected<0?"Original":tm_names[selected],item);
    log_message(text);
}
static uint64_t __cdecl on_frame(void *context) {
    tick();
    /* Always run the normal application frame. Skipping it starves the native
       load/audio services. The game's scheduler filters gameplay while paused. */
    return original_frame(context);
}
/* Invoked via package.loadlib; returns zero Lua values, so no Lua ABI symbols
   are required even when LuaBackend statically links its Lua interpreter. */
__declspec(dllexport) int __cdecl kh1_transmog_bootstrap(void *lua_state) {
    (void)lua_state;
    if (enabled || failed) return 0;
    game=(uintptr_t)GetModuleHandleW(NULL);
    if (!signature_ok()) {failed=1;log_message("Unsupported executable/signature; disabled. Steam Global 1.0.0.2 required.");return 0;}
    resolve_handle=(ResolveProc)(game+0x38ADC0);
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
    if (!install_scheduler_hook()) {
        VirtualProtect(entry,sizeof(*entry),protection,&protection);
        failed=1;log_message("Cannot protect final scheduler call; disabled.");return 0;
    }
    original_frame=(FrameProc)InterlockedExchangePointer((PVOID volatile *)entry,(PVOID)&on_frame);
    VirtualProtect(entry,sizeof(*entry),protection,&protection);
    enabled=1;
    log_message("Frame and final scheduler hooks installed (0.1.12). Waiting for weapon table.");
    return 0;
}
