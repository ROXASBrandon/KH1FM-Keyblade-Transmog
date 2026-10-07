/* Private-memory Windows regression tests; never opens a game process. */
#undef NDEBUG
#include <assert.h>
#include "seamless.c"
static unsigned char actual_weapon[0x60],original_model[0x60],packet[0x60];
static unsigned char *raw_assets[MODEL_COUNT],*sound_assets[MODEL_COUNT];
static int sound_alive[MODEL_COUNT];
static void (__fastcall *bank_completion)(void *);
static void *bank_user;
static int bank_count,observed_sound,observed_tag;
static void *observed_bank;
static int resource_alive[MODEL_COUNT],load_count,init_count,submitted,forwarded;
static void (__fastcall *completion)(int,int,void *);
static uintptr_t expected_draw;
static unsigned char weapon_before[0x60];
static void *__fastcall fake_resolve(uint32_t h) {
    if(h==1)return actual_weapon;if(h==2)return original_model;
    if(h>=3 && h<3+MODEL_COUNT)return raw_assets[h-3]+*(uint32_t *)(raw_assets[h-3]+8)+0x40;
    if(h>=3+MODEL_COUNT && h<3+MODEL_COUNT*2)return raw_assets[h-3-MODEL_COUNT]+0x220;
    return NULL;
}
static void *__fastcall fake_resource(void *p,int mutate) {
    assert(!mutate);
    for(int i=0;i<MODEL_COUNT;++i)if(resource_alive[i] && p==raw_assets[i])return (void *)(uintptr_t)(0x8000+i);
    for(int i=0;i<MODEL_COUNT;++i)if(sound_alive[i] && p==sound_assets[i])return (void *)(uintptr_t)(0x9000+i);
    return NULL;
}
static void __fastcall fake_init(void *p) {
    ++init_count;
    for(int i=0;i<MODEL_COUNT;++i)if(p==raw_assets[i]+*(uint32_t *)(raw_assets[i]+8)) {
        *(uint32_t *)((char *)p+0x20)=3+i;
        *(uint32_t *)((char *)p+0x50)=0x96969696;return;
    }
    assert(0);
}
static void __fastcall fake_effect(void *p) {
    for(int i=0;i<MODEL_COUNT;++i)if(p==raw_assets[i]+0x80) {
        for(int k=0;k<9;++k)*(uint32_t *)((char *)p+16+k*32)=3+MODEL_COUNT+i;
        return;
    }
    assert(0);
}
static int __fastcall fake_bank(void *p,int bank,void (__fastcall *cb)(void *),void *user) {
    int i=bank-0x6F00;assert(i>=0 && i<MODEL_COUNT && p==sound_assets[i]);
    uintptr_t slot=game+0x2D53AB0+bank_count*16;
    *(int *)slot=bank;*(uintptr_t *)(slot+8)=(uintptr_t)p;
    for(int k=0;k<40;++k) {
        uintptr_t node=game+0x2300000+(i*40+k)*0x50;
        *(uintptr_t *)(node+8)=ptr(game+0x4D65D8);
        *(uintptr_t *)(game+0x4D65D8)=node;
        *(int *)(node+0x10)=bank;*(uint32_t *)(node+0x14)=k<35?cosmetic_sound_base[i]+k:0x2B0A+k-35;
        *(uintptr_t *)(node+0x20)=game+0x2400000;
    }
    ++bank_count;bank_completion=cb;bank_user=user;return 1;
}
static void *__fastcall fake_sound(int id,int tag) {
    observed_sound=id;observed_tag=tag;assert(!memcmp(actual_weapon,weapon_before,sizeof(actual_weapon)));
    assert(!u32(game+0x2867370));return (void *)0x11223344;
}
static void *__fastcall fake_trail(void *a,void *bank,int index) {
    (void)a;assert(index==0 || index==99);observed_bank=bank;
    assert(!memcmp(actual_weapon,weapon_before,sizeof(actual_weapon)));return (void *)0x55667788;
}
static int __fastcall fake_load(const char *name,void *dest,void (__fastcall *cb)(int,int,void *),int flags) {
    assert(!dest&&!flags&&load_count<MODEL_COUNT*2);
    assert(!strcmp(name,load_count<MODEL_COUNT?asset_names[load_count]:sound_names[load_count-MODEL_COUNT]));
    completion=cb;return ++load_count;
}
static uint64_t __fastcall fake_submit(void *model,void *matrices,void *data,void *actor) {
    assert(model==(void *)0x1234 && matrices==(void *)0x5678 && data==packet);
    assert(ptr((uintptr_t)data+0x18)==expected_draw);
    assert(u32((uintptr_t)actor+0x14C)==1);
    assert(!memcmp(actual_weapon,weapon_before,sizeof(actual_weapon)));
    assert(u32(game+0x2867370)==0);
    ++submitted;return 0x123456789ABCDEF0ull;
}
static uint64_t __cdecl fake_frame(void *ctx) {assert(ctx==(void *)0x4567);++forwarded;return 0x4321;}
static void raw_reset(void) {
    for(int i=0;i<MODEL_COUNT;++i){
        memset(raw_assets[i],0,expected_size[i]);memcpy(raw_assets[i],expected_wpn[i],16);
        memcpy(raw_assets[i]+*(uint32_t *)(raw_assets[i]+8),expected_model[i],56);resource_alive[i]=1;
        memcpy(raw_assets[i]+0x80,expected_effect[i],16);
        for(int k=0;k<9;++k){*(uint32_t *)(raw_assets[i]+0x90+k*32)=0x1A0;*(uint32_t *)(raw_assets[i]+0x98+k*32)=k;}
        memset(sound_assets[i],0,expected_sound_size[i]);memcpy(sound_assets[i],expected_sound[i],16);sound_alive[i]=1;
    }
}
int main(void) {
    game=(uintptr_t)VirtualAlloc(NULL,0x4000000,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);assert(game);
    for(int i=0;i<MODEL_COUNT;++i){raw_assets[i]=VirtualAlloc(NULL,expected_size[i],MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);assert(raw_assets[i]);sound_assets[i]=VirtualAlloc(NULL,expected_sound_size[i],MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);assert(sound_assets[i]);}
    raw_reset();resolve=fake_resolve;load_file=fake_load;init_model=fake_init;resource_for=fake_resource;original_submit=fake_submit;original_frame=fake_frame;init_effect=fake_effect;install_bank=fake_bank;original_sound=fake_sound;original_trail=fake_trail;
    uintptr_t actor=game+0x2000000,save=game+0x2100000;
    *(uintptr_t *)(game+0x2D37280)=actor;*(uintptr_t *)(game+0x2868BA0)=save;
    *(unsigned char *)(save+0x36)=86;*(uint32_t *)(actor+0x374)=1;*(uint32_t *)(actor+0x130)=99;
    *(uint32_t *)(actor+0x14C)=1;*(uint32_t *)actual_weapon=2;
    memset(actual_weapon+4,0x5A,sizeof(actual_weapon)-4);*(uintptr_t *)(actual_weapon+0x18)=game+0x2200000;
    *(uint32_t *)(game+0x528060)=0x1000;
    *(uintptr_t *)(actual_weapon+0x20)=game+0x2D23D40+5*0x58;
    *(uint32_t *)(ptr((uintptr_t)actual_weapon+0x20)+0x34)=0x2CA0;
    memcpy(weapon_before,actual_weapon,sizeof(actual_weapon));
    *(unsigned char *)(game+0x2D5CC4C)=100;*(float *)(game+0x281249C)=1;
    *(uint32_t *)(game+0x296B540)=5;*(uintptr_t *)(packet+0x18)=0xBBBB;
    ULONGLONG now=GetTickCount64();
    tick_inputs(0,0,1,now);assert(!load_count);
    tick_inputs(1,0,1,now+500);assert(load_count==1 && seamless_phase==LOADING);
    expected_draw=0xBBBB;
    on_submit((void *)0x1234,(void *)0x5678,packet,(void *)actor);assert(!seamless_render_overrides);
    for(int i=0;i<MODEL_COUNT;++i) {
        assert(load_count==i+1);
        completion(expected_size[i],i+1,raw_assets[i]);
        assert(seamless_ready_mask==((1u<<(i+1))-1) && init_count==i+1 && seamless_phase==LOADING);
        tick_inputs(1,0,1,now+510+i);
    }
    for(int i=0;i<MODEL_COUNT;++i) {
        assert(load_count==MODEL_COUNT+i+1);
        completion(expected_sound_size[i],MODEL_COUNT+i+1,sound_assets[i]);
        assert(bank_count==i+1 && seamless_phase==LOADING);
        tick_inputs(1,0,1,now+550+i);assert(load_count==MODEL_COUNT+i+1);
        bank_completion(bank_user);
        assert(seamless_sound_ready_mask==((1u<<(i+1))-1));
        if(i+1<MODEL_COUNT)tick_inputs(1,0,1,now+580+i);
    }
    assert(seamless_phase==READY && seamless_ready_mask==ALL_MODELS_MASK &&
           seamless_sound_ready_mask==ALL_MODELS_MASK && bank_count==MODEL_COUNT);
    for(int i=0;i<MODEL_COUNT;++i)assert(!reserve_bank(i));
    tick_inputs(1,0,1,now+700);assert(selected==-1); /* Held preload press is consumed. */
    tick_inputs(0,0,1,now+701);
    *(uint32_t *)(actor+0x164)=200; /* Mid-swing is permitted with no native reload. */
    tick_inputs(1,0,1,now+702);assert(selected==0 && seamless_swaps==1);
    tick_inputs(1,0,1,now+703);assert(seamless_swaps==1);
    tick_inputs(0,0,1,now+704);tick_inputs(1,0,1,now+705);assert(selected==1 && seamless_swaps==2);
    tick_inputs(0,0,1,now+706);tick_inputs(1,1,1,now+707);assert(selected==-1);
    tick_inputs(0,0,1,now+708);tick_inputs(1,0,0,now+709);tick_inputs(1,0,1,now+710);assert(selected==-1);
    selected=-1;
    for(int n=0;n<MODEL_COUNT*3;++n) {
        tick_inputs(0,0,1,now+711);tick_inputs(1,0,1,now+712);
        assert(selected==n%MODEL_COUNT && load_count==MODEL_COUNT*2);
    }
    /* Thousands of draw substitutions preserve every weapon byte, attachment,
       packet fields, equipped item and pause mask, with no further load calls. */
    for(int n=0;n<4096;++n){
        selected=n%MODEL_COUNT;expected_draw=cached[selected].draw;
        assert(on_submit((void *)0x1234,(void *)0x5678,packet,(void *)actor)==0x123456789ABCDEF0ull);
        assert(ptr((uintptr_t)packet+0x18)==0xBBBB && load_count==MODEL_COUNT*2);
    }
    assert(seamless_render_overrides==4096 && !seamless_unexpected_weapon_changes);
    /* Chest/event display keeps its cosmetic blade while input is gated. */
    selected=1;expected_draw=cached[1].draw;
    uint32_t before=seamless_render_overrides,swaps=seamless_swaps;
    *(unsigned char *)(game+0x23AB2D0)=2;*(float *)(game+0x281249C)=0;
    tick_inputs(0,0,1,now+720);tick_inputs(1,0,1,now+721);
    assert(selected==1 && seamless_swaps==swaps);
    on_submit((void *)0x1234,(void *)0x5678,packet,(void *)actor);
    assert(seamless_render_overrides==before+1 && ptr((uintptr_t)packet+0x18)==0xBBBB);
    *(uint32_t *)(game+0x232DFA0)=1;
    on_submit((void *)0x1234,(void *)0x5678,packet,(void *)actor);
    assert(seamless_render_overrides==before+2);
    *(unsigned char *)(game+0x23AB2D0)=0;*(float *)(game+0x281249C)=1;
    *(uint32_t *)(game+0x232DFA0)=0;
    /* Nonstandard story weapon rows and gummi never receive overrides. */
    uintptr_t params=ptr((uintptr_t)actual_weapon+0x20);
    *(uintptr_t *)(actual_weapon+0x20)=game+0x2D23D40+0x58;
    memcpy(weapon_before,actual_weapon,sizeof(actual_weapon));expected_draw=0xBBBB;
    on_submit((void *)0x1234,(void *)0x5678,packet,(void *)actor);
    assert(seamless_render_overrides==before+2);
    *(uintptr_t *)(actual_weapon+0x20)=params;memcpy(weapon_before,actual_weapon,sizeof(actual_weapon));
    *(unsigned char *)(game+0x5075A8)=1;
    on_submit((void *)0x1234,(void *)0x5678,packet,(void *)actor);
    assert(seamless_render_overrides==before+2);
    *(unsigned char *)(game+0x5075A8)=0;expected_draw=cached[selected].draw;
    /* Exercise the real rel32 call + nearby sealed RX relay with native ABI. */
    unsigned char *site=(unsigned char *)(game+SUBMIT_CALL);
    const unsigned char enter[]={0x48,0x83,0xEC,0x28},leave[]={0x48,0x83,0xC4,0x28,0xC3};
    memcpy(site-4,enter,4);memcpy(site,submit_bytes,5);memcpy(site+5,leave,5);
    for(int i=0;i<4;++i)memcpy((void *)(game+effect_sites[i]),effect_calls[i],5);
    *(unsigned char *)(game+effect_sites[3]+1)^=1;
    assert(!install_render_hook() && !memcmp(site,submit_bytes,5) && !relay);
    *(unsigned char *)(game+effect_sites[3]+1)^=1;
    DWORD old;assert(VirtualProtect(site-4,14,PAGE_EXECUTE_READ,&old));assert(install_render_hook());
    original_submit=fake_submit;original_sound=fake_sound;original_trail=fake_trail;SubmitProc patched=(SubmitProc)(site-4);
    assert(patched((void *)0x1234,(void *)0x5678,packet,(void *)actor)==0x123456789ABCDEF0ull);
    assert(ptr((uintptr_t)packet+0x18)==0xBBBB);assert(!install_render_hook());
    /* Both cosmetic banks preserve every native sound variant/tag and weapon
       byte; out-of-range variants, companions, reset and stale banks pass through. */
    for(int i=0;i<MODEL_COUNT;++i) {
        selected=i;
        for(int variant=0;variant<35;++variant) {
            assert(on_sound(0x2CA0+variant,17,(void *)actor)==(void *)0x11223344);
            assert(observed_sound==(int)cosmetic_sound_base[i]+variant && observed_tag==17);
        }
        for(int variant=35;variant<40;++variant) {
            on_sound(0x2CA0+variant,18,(void *)actor);assert(observed_sound==0x2CA0+variant);
        }
        on_sound(0x2CA0-1,19,(void *)actor);assert(observed_sound==0x2CA0-1);
        on_sound(0x2CA0,20,(void *)(actor+0x1000));assert(observed_sound==0x2CA0);
        assert(on_trail((void *)actor,(void *)ptr((uintptr_t)actual_weapon+0x18),0)==(void *)0x55667788);
        assert(observed_bank==(void *)cached[i].effects);
        on_trail((void *)actor,(void *)ptr((uintptr_t)actual_weapon+0x18),99);
        assert(observed_bank==(void *)ptr((uintptr_t)actual_weapon+0x18));
        on_trail((void *)actor,(void *)0x7788,0);assert(observed_bank==(void *)0x7788);
        on_trail((void *)(actor+0x1000),(void *)0x7788,0);assert(observed_bank==(void *)0x7788);
    }
    assert(seamless_sound_overrides==MODEL_COUNT*35 && seamless_trail_overrides==MODEL_COUNT);
    /* Every equipped weapon uses its own base; cosmetic remapping must never
       assume Kingdom Key stats/sounds or overwrite any parameter bytes. */
    for(int i=0;i<MODEL_COUNT;++i) {
        unsigned row=i?i+4:0;uintptr_t current=game+0x2D23D40+row*0x58;
        *(unsigned char *)(save+0x36)=i?85+i:81;
        unsigned char parameters_before[0x58];memset((void *)current,0xA5,0x58);
        *(uint32_t *)(current+0x34)=cosmetic_sound_base[i];
        memcpy(parameters_before,(void *)current,0x58);
        *(uintptr_t *)(actual_weapon+0x20)=current;
        memcpy(weapon_before,actual_weapon,sizeof(actual_weapon));selected=(i+7)%MODEL_COUNT;
        on_sound((int)cosmetic_sound_base[i]+13,31,(void *)actor);
        assert(observed_sound==(int)cosmetic_sound_base[selected]+13 && observed_tag==31);
        assert(!memcmp(parameters_before,(void *)current,0x58));
    }
    *(unsigned char *)(save+0x36)=86;
    *(uintptr_t *)(actual_weapon+0x20)=params;*(uint32_t *)(params+0x34)=0x2CA0;
    memcpy(weapon_before,actual_weapon,sizeof(actual_weapon));
    selected=1;
    for(int i=0;i<2;++i) {
        unsigned char *call=(unsigned char *)(game+effect_sites[i]);DWORD prior;
        assert(VirtualProtect(call-4,14,PAGE_EXECUTE_READWRITE,&prior));
        memcpy(call-4,enter,4);memcpy(call+5,leave,5);
        assert(VirtualProtect(call-4,14,PAGE_EXECUTE_READ,&prior));
        assert(((TrailProc)(call-4))((void *)actor,(void *)ptr((uintptr_t)actual_weapon+0x18),0)==(void *)0x55667788);
        assert(observed_bank==(void *)cached[1].effects);
    }
    selected=-1;on_sound(0x2CA0,20,(void *)actor);assert(observed_sound==0x2CA0);
    selected=1;sound_alive[1]=0;on_sound(0x2CA0,20,(void *)actor);assert(observed_sound==0x2CA0);sound_alive[1]=1;
    uintptr_t audio_root=ptr(game+0x4D65D8);
    *(uintptr_t *)(game+0x4D65D8)=0;
    on_sound(0x2CA0,20,(void *)actor);assert(observed_sound==0x2CA0);
    *(uintptr_t *)(game+0x4D65D8)=audio_root;
    /* Execute both real sound relays: their RDI/RSI actor extraction must obey
       x64 ABI and preserve nonvolatile registers and original return values. */
    for(int i=2;i<4;++i) {
        unsigned char *call=(unsigned char *)(game+effect_sites[i]);
        unsigned char *entry=call-8;
        unsigned char prefix[8]={i==2?0x57:0x56,0x4C,0x89,i==2?0xC7:0xC6,0x48,0x83,0xEC,0x20};
        unsigned char suffix[6]={0x48,0x83,0xC4,0x20,i==2?0x5F:0x5E,0xC3};
        DWORD prior;assert(VirtualProtect(entry,19,PAGE_EXECUTE_READWRITE,&prior));
        memcpy(entry,prefix,8);memcpy(call+5,suffix,6);
        assert(VirtualProtect(entry,19,PAGE_EXECUTE_READ,&prior));
        typedef void *(__fastcall *SoundRelayTest)(int,int,void *);
        assert(((SoundRelayTest)entry)(0x2CA0+7,23,(void *)actor)==(void *)0x11223344);
        assert(observed_sound==(int)cosmetic_sound_base[1]+7 && observed_tag==23);
    }
    expected_draw=0xBBBB;
    /* Native hide state must remain hidden, never force a blade into a scene. */
    *(uintptr_t *)(packet+0x18)=0;expected_draw=0;
    on_submit((void *)0x1234,(void *)0x5678,packet,(void *)actor);
    *(uintptr_t *)(packet+0x18)=0xBBBB;expected_draw=0xBBBB;
    /* Room warp suspends overrides without clearing selection or requesting
       more assets; a newly attached actor receives the same appearance. */
    int retained=selected;uint32_t draws=seamless_render_overrides;
    *(unsigned char *)(game+0x22EC0AC)=1;
    tick_inputs(0,0,1,now+750);
    assert(seamless_phase==SUSPENDED && selected==retained);
    expected_draw=cached[selected].draw;
    on_submit((void *)0x1234,(void *)0x5678,packet,(void *)actor);
    assert(seamless_render_overrides==draws+1);
    uintptr_t new_actor=actor+0x1000;memcpy((void *)new_actor,(void *)actor,0x4B0);
    *(uint32_t *)(new_actor+0x130)=100;
    *(uintptr_t *)(game+0x2D37280)=new_actor;
    /* New actor renders cosmetically before HUD, warp, or settle timer clears.
       A queued submission from the old actor remains native. */
    on_submit((void *)0x1234,(void *)0x5678,packet,(void *)new_actor);
    assert(seamless_render_overrides==draws+2);
    expected_draw=0xBBBB;
    on_submit((void *)0x1234,(void *)0x5678,packet,(void *)actor);
    assert(seamless_render_overrides==draws+2);
    expected_draw=cached[selected].draw;
    *(float *)(game+0x281249C)=0;
    tick_inputs(1,0,1,now+750);assert(seamless_phase==SUSPENDED);
    on_submit((void *)0x1234,(void *)0x5678,packet,(void *)new_actor);
    assert(seamless_render_overrides==draws+3);
    *(uintptr_t *)(packet+0x18)=0;expected_draw=0;
    on_submit((void *)0x1234,(void *)0x5678,packet,(void *)new_actor);
    assert(seamless_render_overrides==draws+3);
    *(uintptr_t *)(packet+0x18)=0xBBBB;expected_draw=cached[selected].draw;
    *(float *)(game+0x281249C)=1;
    *(unsigned char *)(game+0x22EC0AC)=0;
    tick_inputs(1,0,1,now+751);assert(seamless_phase==SUSPENDED);
    tick_inputs(1,0,1,now+1250);assert(seamless_phase==SUSPENDED);
    tick_inputs(1,0,1,now+1251);
    assert(seamless_phase==READY && selected==retained && seamless_scene_resumes==1 && load_count==MODEL_COUNT*2);
    expected_draw=cached[selected].draw;
    on_submit((void *)0x1234,(void *)0x5678,packet,(void *)new_actor);
    assert(seamless_render_overrides==draws+4 && ptr((uintptr_t)packet+0x18)==0xBBBB);
    expected_draw=0xBBBB;
    /* Even a reused actor address/model cannot bypass the warp suspension. */
    *(unsigned char *)(game+0x22EC0AC)=1;tick_inputs(0,0,1,now+1300);
    *(unsigned char *)(game+0x22EC0AC)=0;tick_inputs(0,0,1,now+1301);
    tick_inputs(0,0,1,now+1801);
    assert(seamless_phase==READY && selected==retained && seamless_scene_resumes==2);
    /* Death/retry clears legacy sound-task slots but leaves PC audio/file
       ownership live. Pause draws at zero HP, retain selection and rebind a
       replacement actor without another file request or equipment reload. */
    *(unsigned char *)(game+0x2D5CC4C)=0;tick_inputs(0,0,1,now+1802);
    assert(seamless_phase==SUSPENDED && selected==retained);
    uint32_t death_draws=seamless_render_overrides;
    on_submit((void *)0x1234,(void *)0x5678,packet,(void *)new_actor);
    assert(seamless_render_overrides==death_draws);
    memset((void *)(game+0x2D53AB0),0,128*16);
    assert(sound_valid(&sounds[0]) && sound_valid(&sounds[MODEL_COUNT-1]));
    assert(!reserve_bank(0)); /* PC group still occupied despite cleared slots. */
    uintptr_t retry_actor=actor+0x2000;memcpy((void *)retry_actor,(void *)new_actor,0x4B0);
    *(uint32_t *)(retry_actor+0x130)=101;
    *(uintptr_t *)(game+0x2D37280)=retry_actor;
    *(unsigned char *)(game+0x2D5CC4C)=100;
    tick_inputs(1,0,1,now+1803);tick_inputs(1,0,1,now+2303);
    assert(seamless_phase==READY && selected==retained && seamless_scene_resumes==3 && load_count==MODEL_COUNT*2);
    expected_draw=cached[selected].draw;
    on_submit((void *)0x1234,(void *)0x5678,packet,(void *)retry_actor);
    on_sound(0x2CA0+3,17,(void *)retry_actor);
    assert(observed_sound==(int)cosmetic_sound_base[selected]+3);
    on_trail((void *)retry_actor,(void *)ptr((uintptr_t)actual_weapon+0x18),0);
    assert(observed_bank==(void *)cached[selected].effects);
    expected_draw=0xBBBB;new_actor=retry_actor;
    tick_inputs(0,0,1,now+2304); /* Held Q during retry remains consumed. */
    /* Shift+Q reset also survives a transition. */
    tick_inputs(1,1,1,now+2305);assert(selected==-1);
    *(unsigned char *)(game+0x22EC0AC)=1;tick_inputs(0,0,1,now+2306);
    *(unsigned char *)(game+0x22EC0AC)=0;tick_inputs(0,0,1,now+2307);
    tick_inputs(0,0,1,now+2807);assert(seamless_phase==READY && selected==-1);
    selected=retained;
    resource_alive[selected]=0;
    on_submit((void *)0x1234,(void *)0x5678,packet,(void *)new_actor);
    assert(seamless_phase==INVALIDATED && selected==-1);
    tick_inputs(0,0,1,now+800);tick_inputs(1,0,1,now+801);assert(load_count==MODEL_COUNT*2);
    assert(on_frame((void *)0x4567)==0x4321 && forwarded==1); /* Always progress engine. */
    /* Unloaded resources at destination permanently disable without reloads. */
    seamless_phase=SUSPENDED;selected=retained;idle_tracking=0;
    tick_inputs(0,0,1,now+2400);tick_inputs(0,0,1,now+2900);
    assert(seamless_phase==INVALIDATED && selected==-1 && load_count==MODEL_COUNT*2);
    /* Late callback after actor replacement cannot initialize stale assets. */
    *(uintptr_t *)(game+0x2D37280)=actor;owner_actor=actor;owner_model=99;
    seamless_phase=LOADING;loading_index=0;request_id=1;
    *(uint32_t *)(actor+0x130)=100;completion(expected_size[0],1,raw_assets[0]);
    assert(seamless_phase==INVALIDATED && init_count==MODEL_COUNT);
    *(uint32_t *)(actor+0x130)=99;raw_reset();seamless_phase=LOADING;loading_index=0;
    completion(expected_size[0],999,raw_assets[0]);assert(init_count==MODEL_COUNT); /* Wrong request. */
    raw_assets[0][0]^=1;completion(expected_size[0],1,raw_assets[0]);assert(seamless_phase==FAILED && init_count==MODEL_COUNT);
    raw_reset();seamless_phase=LOADING;completion(expected_size[0]-1,1,raw_assets[0]);assert(seamless_phase==FAILED && init_count==MODEL_COUNT);
    raw_reset();seamless_phase=LOADING;resource_alive[0]=0;completion(expected_size[0],1,raw_assets[0]);assert(seamless_phase==FAILED && init_count==MODEL_COUNT);
    raw_reset();seamless_phase=LOADING;
    *(uint32_t *)(raw_assets[0]+0x8C)=10;
    completion(expected_size[0],1,raw_assets[0]);assert(seamless_phase==FAILED && init_count==MODEL_COUNT);
    raw_reset();seamless_phase=LOADING;loading_index=MODEL_COUNT;request_id=MODEL_COUNT+1;
    sound_assets[0][0]^=1;completion(expected_sound_size[0],MODEL_COUNT+1,sound_assets[0]);assert(seamless_phase==FAILED);
    raw_reset();seamless_phase=LOADING;loading_index=MODEL_COUNT;
    completion(expected_sound_size[0],MODEL_COUNT+1,sound_assets[0]);assert(seamless_phase==FAILED && bank_count==MODEL_COUNT); /* Group collision. */
    seamless_phase=LOADING;loading_index=MODEL_COUNT;*(uintptr_t *)(game+0x4D65D8)=0;
    sound_ready(&sounds[0]);assert(seamless_phase==INVALIDATED); /* Missing PC clips. */
    uint32_t mask=seamless_sound_ready_mask;sound_ready(&sounds[0]);assert(seamless_sound_ready_mask==mask);
    seamless_phase=LOADING;loading_index=0;load_started=now;tick_inputs(0,0,1,now+10001);
    assert(seamless_phase==FAILED && u32(actor+0x14C)==1 && u32(game+0x2867370)==0);
    assert(on_frame((void *)0x4567)==0x4321 && forwarded==2);
    puts("PASS: 4096 render substitutions preserve attached weapon, combat bytes, packet, native frame progression and pause mask.");
    puts("PASS: transition fade-in renders current attached cosmetic before HUD/settle readiness; old actor, dead actor and hidden packet remain native.");
    puts("PASS: death/retry with cleared KH sound slots retains look/effects and rebinds without loads; PC bank collision guard remains intact.");
    puts("PASS: scene/actor rebinding retains selection/reset, consumes held Q, validates resources, never reloads; unloaded assets disable safely.");
    puts("PASS: 36 bounded asynchronous preloads, all 18 ready bits; mid-swing/held/busy/unfocused Q, reset, hidden blade, stale resources, actor changes during preload, wrong callback, bad asset and timeout.");
    puts("PASS: both sound actor-extraction relays; 630 remapped variants across 18 blades, trail bank substitution, reset/companion/stale/unsupported fallback, no combat writes.");
    puts("PASS: chest/event HUD/cutscene/menu gates retain cosmetic rendering, block Q; story weapons, hidden native blade and gummi pass through.");
    puts("PASS: actual rel32 render hook, native ABI return forwarding and sealed relay; game process never accessed.");
    return 0;
}
