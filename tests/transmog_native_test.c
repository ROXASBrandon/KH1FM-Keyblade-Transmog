/* Windows-only native integration test. Uses isolated memory; never opens or
   patches a running game. Exercises capture/apply/restore and conflict guards. */
#undef NDEBUG
#include <assert.h>
#include "kh1_transmog.c"
static unsigned char fake_weapon[0x60];
static void *__fastcall resolve_test(uint32_t handle) {
    return handle==1 ? fake_weapon : NULL;
}

static int gameplay_frames, application_frames, load_after;
static uint32_t scheduler_received;
static int render_frames, cleanup_frames, service_frames, unknown_frames;
static int audio_pending, audio_frames;
static void __fastcall scheduler_test(void *group, uint32_t mask) {
    assert(group==(void *)(game+0x2867380));
    scheduler_received=mask;
    uintptr_t actor=*(uintptr_t *)(game+0x2D37280);
    uintptr_t task=*(uintptr_t *)((uintptr_t)group+0x10);
    for (int n=0;task && n<8;++n) {
        uint32_t flags=*(uint16_t *)(task+2);
        uintptr_t callback=*(uintptr_t *)(task+0x10);
        if ((flags&mask)==mask) {
            if (callback==game+0x298940) {
                assert(actor_weapon(actor) && attached_model_ready(actor));
                ++gameplay_frames;
            } else if (callback==game+0x2937F0) {
                assert(actor_weapon(actor));++cleanup_frames;
            } else if (callback==game+AUDIO_QUEUE_TASK) {
                if (audio_pending) --audio_pending;
                ++audio_frames;
            } else if (render_task(callback)) ++render_frames;
            else if (flags==0xFFFF) ++service_frames;
            else ++unknown_frames;
        }
        task=*(uintptr_t *)(task+8);
    }
}
static uint64_t __cdecl frame_test(void *context) {
    assert(context==(void *)0x1234);
    ++application_frames;
    uintptr_t actor=*(uintptr_t *)(game+0x2D37280);
    /* Background asset/audio progression must continue while gameplay pauses. */
    if (load_after && application_frames>=load_after && !audio_pending) {
        *(uint32_t *)(actor+0x14C)=1;
        *(uint32_t *)(game+0x296B540)=5;
        *(uint32_t *)fake_weapon=1;
        *(uintptr_t *)(fake_weapon+0x18)=(uintptr_t)fake_weapon;
    }
    /* Reproduce the real failure: normal pause handling overwrites the global
       after the app-frame hook but before the final task scheduler call. */
    *(uint32_t *)(game+GAMEPLAY_PAUSE_MASK)=0;
    on_scheduler((void *)(game+0x2867380),*(uint32_t *)(game+GAMEPLAY_PAUSE_MASK));
    return 0xABCDEF;
}
int main(void) {
    game=(uintptr_t)VirtualAlloc(NULL,0x4000000,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);
    assert(game);
    /* Private task group: actor, cleanup, camera, model, services, unknown,
       room geometry submission, and the companion audio command updater. */
    uintptr_t tasks=game+0x1000000;
    uintptr_t callbacks[8]={game+0x298940,game+0x2937F0,game+CAMERA_SUBMIT_TASK,
        game+MODEL_SUBMIT_TASK,game+0x286200,game+0x123456,
        game+ROOM_SUBMIT_TASK,game+AUDIO_QUEUE_TASK};
    *(uintptr_t *)(game+0x2867390)=tasks;
    for (int i=0;i<8;++i) {
        uintptr_t node=tasks+i*0x18;
        *(uint16_t *)(node+2)=i==4?0xFFFF:0;
        *(uintptr_t *)(node+8)=i<7?node+0x18:0;
        *(uintptr_t *)(node+0x10)=callbacks[i];
    }
    unsigned char *record_table=(unsigned char *)(game+0x2D22D40+0x10000);
    unsigned char baseline[22*TM_STRIDE];
    for (size_t i=0;i<sizeof(baseline);++i) baseline[i]=(unsigned char)(i*37+11);
    for (int i=0;i<TM_COUNT;++i) {
        unsigned char *record=baseline+tm_rows[i]*TM_STRIDE;
        memset(record,0,TM_MODEL_SIZE);
        memcpy(record,tm_models[i],strlen(tm_models[i]));
        uint32_t sound=0x12000000+(uint32_t)i*0x100; /* Prove all four bytes copy. */
        memcpy(record+TM_SOUND_OFFSET,&sound,TM_SOUND_SIZE);
    }
    memcpy(record_table,baseline,sizeof(baseline));
    *(uintptr_t *)(game+0x2868BA0)=game+0x2000000;
    *(uint32_t *)(game+0x528060)=0x10000;
    assert(capture_table());
    assert(table==(uintptr_t)record_table);
    for (int target=0;target<TM_COUNT;++target) {
        selected=target;apply_model();
        for (int equipped=0;equipped<TM_COUNT;++equipped) {
            unsigned char *record=record_table+tm_rows[equipped]*TM_STRIDE;
            assert(tm_cosmetic_matches(record,originals[target]));
            for (int byte=TM_MODEL_SIZE;byte<TM_STRIDE;++byte)
                if (byte<TM_SOUND_OFFSET || byte>=TM_SOUND_OFFSET+TM_SOUND_SIZE)
                    assert(record[byte]==baseline[tm_rows[equipped]*TM_STRIDE+byte]);
        }
    }
    selected=-1;apply_model();
    assert(!memcmp(record_table,baseline,sizeof(baseline)));
    /* A later stat-only mod must survive both cosmetic application and reset. */
    record_table[0x30]^=0x80;
    selected=3;apply_model();selected=-1;apply_model();
    assert(record_table[0x30]==(unsigned char)(baseline[0x30]^0x80));
    /* A competing sound edit must stop override without clobbering that edit. */
    record_table[TM_SOUND_OFFSET]^=0x40;
    unsigned char other_sound=record_table[TM_SOUND_OFFSET];
    selected=4;apply_model();
    assert(selected==-1 && record_table[TM_SOUND_OFFSET]==other_sound);
    /* Regression: an asynchronous cosmetic reload temporarily clears +0x14C.
       The old loader guard accepted state 1 and allowed empty-hand attacks. */
    uintptr_t actor=game+0x2000200,slot=game+0x296B540;
    resolve_handle=resolve_test;
    *(uintptr_t *)(game+0x2D37280)=actor;
    *(uint32_t *)(actor+0x130)=42;
    *(uint32_t *)(actor+0x14C)=1;
    *(uint32_t *)slot=5;
    *(uint32_t *)fake_weapon=1;
    *(uintptr_t *)(fake_weapon+0x18)=(uintptr_t)fake_weapon;
    assert(swap_ready(actor));
    *(uint32_t *)slot=1;assert(!loader_ready() && !swap_ready(actor));
    *(uint32_t *)slot=3;assert(!swap_ready(actor));
    *(uint32_t *)slot=5;
    /* A completed attack leaves this counter set. Q must remain usable. */
    fake_weapon[0x41]=1;assert(swap_ready(actor));
    fake_weapon[0x41]=255;assert(swap_ready(actor));
    fake_weapon[0x41]=0;
    /* Mid-swing keeps action state 0; actual animation ID must reject Q. */
    for (uint32_t anim=1;anim<512;++anim) {
        *(uint32_t *)(actor+0x164)=anim;assert(!swap_ready(actor));
    }
    *(uint32_t *)(actor+0x164)=0x10000;assert(!swap_ready(actor));
    *(uint32_t *)(actor+0x164)=0;
    *(unsigned char *)(actor+0x88)=0x20;assert(!swap_ready(actor));
    *(unsigned char *)(actor+0x88)=0;
    *(unsigned char *)(actor+0x444)=1;assert(!swap_ready(actor));
    *(unsigned char *)(actor+0x444)=2;assert(!swap_ready(actor));
    *(unsigned char *)(actor+0x444)=0;
    assert(swap_ready(actor));
    *(uint32_t *)(actor+0x70)=2;assert(!swap_ready(actor));
    *(uint32_t *)(actor+0x70)=0;
    *(uint32_t *)(actor+0x14C)=2;assert(!swap_ready(actor));
    *(uint32_t *)(actor+0x14C)=1;
    *(unsigned char *)actor=0x80;
    begin_swap(actor);last_swap=GetTickCount64()-4000;
    assert(*(unsigned char *)actor==0x80);
    assert(*(uint32_t *)(game+GAMEPLAY_PAUSE_MASK)==0);
    *(uint32_t *)(actor+0x14C)=0;
    *(uint32_t *)slot=3;
    assert(!finish_swap() && swap_pending && swap_warned);
    *(uint32_t *)slot=5;
    assert(!finish_swap() && *(unsigned char *)actor==0x80);
    *(uint32_t *)(actor+0x14C)=1;
    *(uint32_t *)fake_weapon=1;
    *(uintptr_t *)(fake_weapon+0x18)=(uintptr_t)fake_weapon;
    assert(finish_swap() && !swap_pending);
    assert(*(unsigned char *)actor==0x80);
    /* New actors retain their own flags during a transition. */
    begin_swap(actor);
    uintptr_t replacement=actor+0x1000;
    *(uintptr_t *)(game+0x2D37280)=replacement;
    *(unsigned char *)replacement=0x24;
    assert(!finish_swap() && !swap_pending);
    assert(*(unsigned char *)replacement==0x24);
    *(uintptr_t *)(game+0x2D37280)=actor;
    *(unsigned char *)actor=0x80;
    begin_swap(actor);*(uint32_t *)(actor+0x130)=43;
    assert(!finish_swap() && !swap_pending && *(unsigned char *)actor==0x80);
    *(uint32_t *)(actor+0x130)=42;
    original_scheduler=scheduler_test;
    original_frame=frame_test;
    assert(frame_test((void *)0x1234)==0xABCDEF && gameplay_frames==1);
    /* Reproduce captured battle deadlock: service tasks run, but native mask
       alone excludes the companion audio updater, so sound installation waits. */
    begin_swap(actor);audio_pending=4;
    *(uint32_t *)(actor+0x14C)=0;*(uint32_t *)slot=3;
    for (int n=0;n<6;++n) {
        scheduler_test((void *)(game+0x2867380),1);
        assert(audio_pending && !finish_swap() && !actor_weapon(actor));
    }
    for (int n=0;n<4;++n) {
        on_scheduler((void *)(game+0x2867380),0);
        assert(audio_pending==3-n && swap_pending && !actor_weapon(actor));
    }
    *(uint32_t *)(actor+0x14C)=1;*(uint32_t *)slot=5;
    assert(finish_swap());
    for (int n=0;n<324;++n) {
        begin_swap(actor);audio_pending=4;last_swap=GetTickCount64();
        *(uint32_t *)(actor+0x14C)=0;
        *(uint32_t *)(game+0x296B540)=3;
        int before_frames=gameplay_frames,before_app=application_frames;
        int before_render=render_frames,before_cleanup=cleanup_frames,before_services=service_frames,before_unknown=unknown_frames;
        load_after=application_frames+5;
        for (int f=0;f<5;++f) {
            assert(!finish_swap());
            assert(original_frame((void *)0x1234)==0xABCDEF);
            assert(gameplay_frames==before_frames && swap_pending);
            assert(*(uint32_t *)(game+GAMEPLAY_PAUSE_MASK)==0 && scheduler_received==1);
        }
        assert(application_frames==before_app+5 && !audio_pending);
        assert(render_frames==before_render+15 && cleanup_frames==before_cleanup);
        assert(service_frames==before_services+5 && unknown_frames==before_unknown);
        for (int i=0;i<8;++i) assert(*(uint16_t *)(tasks+i*0x18+2)==(i==4?0xFFFF:0));
        assert(finish_swap() && !swap_pending);
        assert(*(uint32_t *)(game+GAMEPLAY_PAUSE_MASK)==0);
        assert(original_frame((void *)0x1234)==0xABCDEF);
        assert(gameplay_frames==before_frames+1);
        assert(*(unsigned char *)actor==0x80);
    }
    /* Preserve unrelated native mask bits; never modify the native global. */
    begin_swap(actor);
    *(uint32_t *)(game+GAMEPLAY_PAUSE_MASK)|=4;
    on_scheduler((void *)(game+0x2867380),4);
    assert(scheduler_received==5 && *(uint32_t *)(game+GAMEPLAY_PAUSE_MASK)==4);
    assert(finish_swap());
    on_scheduler((void *)(game+0x2867380),4);
    assert(scheduler_received==4 && *(uint32_t *)(game+GAMEPLAY_PAUSE_MASK)==4);
    assert(!swap_ready(actor));
    *(uint32_t *)(game+GAMEPLAY_PAUSE_MASK)=0;
    /* Exercise the actual patched rel32 call and executable near relay. The
       fake game is private test memory, never a running game process. */
    unsigned char *site=(unsigned char *)(game+SCHEDULER_CALL);
    const unsigned char enter[4]={0x48,0x83,0xEC,0x28};
    const unsigned char leave[5]={0x48,0x83,0xC4,0x28,0xC3};
    memcpy(site-4,enter,4);memcpy(site,scheduler_call_original,5);memcpy(site+5,leave,5);
    DWORD protection;
    assert(VirtualProtect(site-4,14,PAGE_EXECUTE_READ,&protection));
    assert(install_scheduler_hook());
    original_scheduler=scheduler_test;
    SchedulerProc patched=(SchedulerProc)(site-4);
    begin_swap(actor);
    patched((void *)(game+0x2867380),0);assert(scheduler_received==1);
    patched((void *)(game+0x2867380),4);assert(scheduler_received==5);
    assert(finish_swap());
    patched((void *)(game+0x2867380),0);assert(scheduler_received==0);
    patched((void *)(game+0x2867380),4);assert(scheduler_received==4);
    assert(!install_scheduler_hook()); /* Already patched: fail closed. */
    VirtualFree(scheduler_relay,0,MEM_RELEASE);scheduler_relay=NULL;
    /* Own only the injected bit, preserving native flags and pause masks. */
    *(uint16_t *)(tasks+2*0x18+2)=4;
    begin_swap(actor);
    int before_render=render_frames;
    on_scheduler((void *)(game+0x2867380),8);
    assert(render_frames==before_render); /* Native pause still excludes render. */
    assert(*(uint16_t *)(tasks+2*0x18+2)==4);
    assert(finish_swap());
    *(uint16_t *)(tasks+3*0x18+2)=3;
    begin_swap(actor);
    on_scheduler((void *)(game+0x2867380),0);
    assert(*(uint16_t *)(tasks+3*0x18+2)==3); /* Pre-existing bit stays owned by game. */
    assert(finish_swap());
    assert(!reload_task(game+0x298940) && !reload_task(game+0x2937F0));
    assert(audio_frames>0 && kh1_transmog_audio_allowances>0);
    puts("PASS: 324 reloads drain companion audio queue and keep camera/model/room submission and services running; attack/cleanup remain blocked, temporary task bits restored, native masks preserved.");
    puts("PASS: final scheduler rel32 hook executed; 324 delayed reloads survive native mask resets, loading continues, gameplay excluded, unrelated masks preserved.");
    puts("PASS: Windows native capture/apply/reset, all 324 model/sound pairs, combat preservation and conflict guards.");
    VirtualFree((void *)game,0,MEM_RELEASE);
    return 0;
}
