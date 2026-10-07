/* Experimental graphics/effects/sound prototype, Steam Global 1.0.0.2. */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <bcrypt.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "asset_headers.h"
#include "heart_profile.h"

enum { WAITING, LOADING, READY, INVALIDATED, FAILED, SUSPENDED };
typedef void *(__fastcall *ResolveProc)(uint32_t);
typedef int (__fastcall *LoadProc)(const char *,void *,void (__fastcall *)(int,int,void *),int);
typedef void (__fastcall *InitProc)(void *);
typedef int (__fastcall *BankProc)(void *,int,void (__fastcall *)(void *),void *);
typedef void *(__fastcall *SoundProc)(int,int);
typedef void *(__fastcall *TrailProc)(void *,void *,int);
typedef void *(__fastcall *ResourceProc)(void *,int);
typedef uint64_t (__fastcall *SubmitProc)(void *,void *,void *,void *);
typedef uint64_t (__cdecl *FrameProc)(void *);
static uintptr_t game;
static ResolveProc resolve;
static LoadProc load_file;
static InitProc init_model;
static InitProc init_effect;
static BankProc install_bank;
static SoundProc original_sound;
static TrailProc original_trail;
static ResourceProc resource_for;
static SubmitProc original_submit;
static FrameProc original_frame;
static void *relay;
static void *extra_relays[4];
static int enabled,failed,was_q,selected=-1,loading_index=-1;
static int request_id;
static uintptr_t owner_actor;
static uint32_t owner_model;
static ULONGLONG idle_since,load_started;
static int idle_tracking;
struct Cached {uintptr_t raw,model,draw,resource;uint32_t bytes;int index;uintptr_t effects;int profile;};
static struct Cached cached[MODEL_COUNT];
struct SoundCached {uintptr_t raw,resource;uint32_t bytes;int index,bank;};
static struct SoundCached sounds[MODEL_COUNT];
__declspec(dllexport) volatile uint32_t kh1_transmog_build=2008;
__declspec(dllexport) volatile uint32_t seamless_phase=WAITING;
__declspec(dllexport) volatile uint32_t seamless_ready_mask;
__declspec(dllexport) volatile uint32_t seamless_swaps;
__declspec(dllexport) volatile uint32_t seamless_render_overrides;
__declspec(dllexport) volatile uint32_t seamless_frame_count;
__declspec(dllexport) volatile uint32_t seamless_loads;
__declspec(dllexport) volatile uint32_t seamless_unexpected_weapon_changes;
__declspec(dllexport) volatile uint32_t seamless_scene_resumes;
__declspec(dllexport) volatile uint32_t seamless_sound_ready_mask;
__declspec(dllexport) volatile uint32_t seamless_failure_reason;
__declspec(dllexport) volatile uint32_t seamless_preload_step;
__declspec(dllexport) volatile uint32_t seamless_sound_overrides;
__declspec(dllexport) volatile uint32_t seamless_trail_overrides;
#define SUBMIT_CALL 0x2A1A08
static const unsigned char submit_bytes[5]={0xE8,0x03,0x4B,0xF3,0xFF};
static int readable(uintptr_t p,size_t n) {
    MEMORY_BASIC_INFORMATION m;
    return p && p+n>=p && VirtualQuery((void *)p,&m,sizeof(m)) &&
        m.State==MEM_COMMIT && !(m.Protect&(PAGE_GUARD|PAGE_NOACCESS)) &&
        p+n<=(uintptr_t)m.BaseAddress+m.RegionSize;
}
static int within(uintptr_t p,size_t n,uintptr_t start,size_t size) {
    return p>=start && n<=size && p-start<=size-n;
}
static uint32_t u32(uintptr_t p) {return *(uint32_t *)p;}
static uintptr_t ptr(uintptr_t p) {return *(uintptr_t *)p;}
static int bytes_at(uintptr_t rva,const unsigned char *b,size_t n) {
    return readable(game+rva,n) && !memcmp((void *)(game+rva),b,n);
}
static void report(const char *s) {printf("[Seamless Prototype] %s\n",s);fflush(stdout);}
static int standard_equipment(void) {
    uintptr_t save=ptr(game+0x2868BA0);
    if (!readable(save,0x74)) return 0;
    unsigned item=*(unsigned char *)(save+0x36);
    return item==81 || (item>=86 && item<=102);
}
static int gameplay(void) {
    uintptr_t actor=ptr(game+0x2D37280);
    return readable(actor,0x4B0) && (u32(actor+0x374)&3)==1 &&
        *(unsigned char *)(game+0x2D5CC4C)>0 && !u32(game+0x232DFA0) &&
        !*(unsigned char *)(game+0x22EC0AC) && !*(unsigned char *)(game+0x23AB2D0) &&
        !*(unsigned char *)(game+0x5075A8) && *(float *)(game+0x281249C)>0.5f;
}
static uintptr_t weapon(uintptr_t actor) {
    if (!readable(actor,0x150)) return 0;
    uintptr_t w=(uintptr_t)resolve(u32(actor+0x14C));
    return readable(w,0x60)?w:0;
}
static int attached(uintptr_t actor) {
    uintptr_t w=weapon(actor);
    return w && u32(w) && readable((uintptr_t)resolve(u32(w)),0x38) &&
        (u32(game+0x296B540)&7)==5 && !ptr(game+0x296B5D0);
}
static void stop_cache(int failure,const char *why) {
    selected=-1;seamless_ready_mask=0;seamless_sound_ready_mask=0;
    seamless_phase=failure?FAILED:INVALIDATED;
    report(why);
    /* Native file/resource system owns these buffers. Do not free resources
       that may still be referenced by a queued renderer or native file task.
       This bounded prototype makes at most 36 requests and never retries. */
}
static int context_valid(void) {
    uintptr_t a=ptr(game+0x2D37280);
    return a==owner_actor && readable(a,0x150) && u32(a+0x130)==owner_model &&
        !*(unsigned char *)(game+0x22EC0AC) && *(unsigned char *)(game+0x2D5CC4C)>0;
}
/* Vanilla checks remain unchanged. Only index 0 can use this exact custom file.
   Hash before native initialization, which replaces relative offsets with handles. */
static int heart_hash_matches(const void *data,uint32_t size) {
    BCRYPT_ALG_HANDLE alg=NULL;BCRYPT_HASH_HANDLE hash=NULL;unsigned char digest[32];int ok=0;
    if (BCryptOpenAlgorithmProvider(&alg,BCRYPT_SHA256_ALGORITHM,NULL,0)<0) return 0;
    if (BCryptCreateHash(alg,&hash,NULL,0,NULL,0,0)>=0 &&
        BCryptHashData(hash,(PUCHAR)data,size,0)>=0 &&
        BCryptFinishHash(hash,digest,sizeof(digest),0)>=0)
        ok=!memcmp(digest,heart_sha256,sizeof(digest));
    if (hash) BCryptDestroyHash(hash);
    BCryptCloseAlgorithmProvider(alg,0);return ok;
}
static int weapon_profile(int index,int size,uintptr_t raw) {
    if (index<0 || index>=MODEL_COUNT || size<56 || !readable(raw,(size_t)size)) return -1;
    if (size==(int)expected_size[index] && !memcmp((void *)raw,expected_wpn[index],16)) return 0;
    if (index==0 && size==(int)heart_size && !memcmp((void *)raw,heart_wpn,16) &&
        heart_hash_matches((void *)raw,(uint32_t)size)) return 1;
    return -1;
}
static const unsigned char *profile_header(int index,int profile) {
    return profile==1 && index==0?heart_wpn:expected_wpn[index];
}
static int cached_valid(const struct Cached *c) {
    /* Registry membership is checked before dereferencing borrowed graphics.
       Comparing the original resource record prevents reuse after unload. */
    return c->raw && (uintptr_t)resource_for((void *)c->raw,0)==c->resource &&
        readable(c->raw,c->bytes) && within(c->draw,0x20,c->raw,c->bytes) &&
        within(c->model,0x38,c->raw,c->bytes) &&
        c->index>=0 && c->index<MODEL_COUNT && (c->profile==0 || (c->profile==1 && c->index==0)) &&
        c->bytes==(c->profile?heart_size:expected_size[c->index]) &&
        !memcmp((void *)c->raw,profile_header(c->index,c->profile),16) &&
        u32(c->model)==0x564E454D && (uintptr_t)resolve(u32(c->model+0x20))==c->draw &&
        u32(c->draw+0x10)==0x96969696 && within(c->effects,16,c->raw,c->bytes) &&
        !memcmp((void *)c->effects,expected_effect[c->index],16);
}
static int sound_source_valid(const struct SoundCached *c) {
    return c->raw && c->index>=0 && c->index<MODEL_COUNT &&
        (uintptr_t)resource_for((void *)c->raw,0)==c->resource &&
        readable(c->raw,c->bytes) && !memcmp((void *)c->raw,expected_sound[c->index],16);
}
static int reserve_bank(int index) {
    /* Separate unload groups; never replace native weapon groups -3/-4/-5.
       Refuse occupied IDs in both KH's task table and PC audio registry. */
    int bank=0x6F00+index,free_slot=0;
    for (int i=0;i<128;++i) {
        uintptr_t slot=game+0x2D53AB0+i*16;
        if (!ptr(slot+8)) free_slot=1;
        else if ((int)u32(slot)==bank) return 0;
    }
    uintptr_t node=ptr(game+0x4D65D8);
    for (int i=0;node && i<4096;++i) {
        if (!readable(node,0x50) || (int)u32(node+0x10)==bank) return 0;
        node=ptr(node+8);
    }
    return !node && free_slot?bank:0;
}
static uint64_t audio_mask(int index) {
    uintptr_t node=ptr(game+0x4D65D8);uint64_t mask=0;
    for (int i=0;node && i<4096;++i) {
        if (!readable(node,0x50)) return 0;
        uint32_t variant=u32(node+0x14)-cosmetic_sound_base[index];
        if ((int)u32(node+0x10)==sounds[index].bank && variant<35 && ptr(node+0x20))
            mask|=1ull<<variant;
        node=ptr(node+8);
    }
    return node?0:mask;
}
static int sound_valid(const struct SoundCached *c) {
    /* Death/retry clears KH's 0x2D53AB0 bookkeeping slots while the PC audio
       registry and loaded file resources remain alive. The actual source
       resource plus all 35 registered hit clips prove usable ownership;
       a historical task-table entry is not a lifetime guarantee. */
    return sound_source_valid(c) && audio_mask(c->index)==((1ull<<35)-1);
}
static int sound_available(int index,unsigned variant) {
    uintptr_t node=ptr(game+0x4D65D8);
    for (int i=0;node && i<4096;++i) {
        if (!readable(node,0x50)) return 0;
        if ((int)u32(node+0x10)==sounds[index].bank &&
            u32(node+0x14)==cosmetic_sound_base[index]+variant && ptr(node+0x20)) return 1;
        node=ptr(node+8);
    }
    return 0;
}
static int effect_layout(uintptr_t raw,uint32_t size,uint32_t offset,uint32_t end) {
    if (offset>end || end>size || end-offset<20) return 0;
    uintptr_t bank=raw+offset;uint32_t count=u32(bank+12);
    if (count!=9 || memcmp((void *)bank,expected_effect[0],16) ||
        end-offset<20+count*32) return 0;
    for (uint32_t i=0;i<count;++i) {
        uint32_t entry=u32(bank+16+i*32);
        if (entry && (entry>=end-offset || end-offset-entry<16)) return 0;
    }
    uint32_t count_offset=16+count*32,n=u32(bank+count_offset);
    if (n>(end-offset-count_offset-4)/4) return 0;
    for (uint32_t i=0;i<n;++i) {
        uint32_t entry=u32(bank+count_offset+4+i*4);
        if (entry>=end-offset || end-offset-entry<16) return 0;
    }
    return 1;
}
static void ready_if_complete(void) {
    if (seamless_ready_mask==ALL_MODELS_MASK && seamless_sound_ready_mask==ALL_MODELS_MASK) {
        seamless_phase=READY;
        report("Ready experimental 0.2.0-p8: Q cycles all 18 Keyblade graphics, new trails and hit sounds; Shift+Q resets.");
    }
}
static void __fastcall sound_ready(void *user) {
    struct SoundCached *c=(struct SoundCached *)user;
    if (seamless_phase!=LOADING || loading_index<MODEL_COUNT || loading_index>=MODEL_COUNT*2 || c!=&sounds[loading_index-MODEL_COUNT]) return;
    if (!context_valid()) {
        seamless_failure_reason=1;stop_cache(0,"Scene changed during sound preload; restart to retry.");return;
    }
    if (!sound_source_valid(c)) {
        seamless_failure_reason=2;stop_cache(0,"Sound ownership changed during preload; restart to retry.");return;
    }
    /* Each bank has 35 blade-specific hits (base..base+34) plus five shared
       swing clips at 0x2B0A..0x2B0E. Shared clips are not hit-base variants. */
    if (audio_mask(c->index)!=((1ull<<35)-1)) {
        seamless_failure_reason=3;stop_cache(0,"Cosmetic bank lacks required hit clips; restart to retry.");return;
    }
    seamless_sound_ready_mask|=1u<<c->index;loading_index=-1;ready_if_complete();
}
static void __fastcall loaded(int size,int id,void *data) {
    if (seamless_phase!=LOADING || loading_index<0 || id!=request_id) return;
    if (!context_valid()) {stop_cache(0,"Scene changed while preloading; vanilla rendering until restart.");return;}
    int index=loading_index;uintptr_t raw=(uintptr_t)data;
    if (index>=MODEL_COUNT) {
        index-=MODEL_COUNT;
        if (index>=MODEL_COUNT || size!=(int)expected_sound_size[index] || !readable(raw,(size_t)size) ||
            memcmp(data,expected_sound[index],16)) {
            stop_cache(1,"Unexpected cosmetic sound asset; disabled.");return;
        }
        uintptr_t resource=(uintptr_t)resource_for(data,0);int bank=reserve_bank(index);
        if (!resource || !bank) {stop_cache(1,"Sound ownership/group unavailable; disabled.");return;}
        sounds[index]=(struct SoundCached){raw,resource,(uint32_t)size,index,bank};
        if (!install_bank(data,bank,sound_ready,&sounds[index]))
            stop_cache(1,"Native sound preload failed; disabled.");
        return;
    }
    int profile=weapon_profile(index,size,raw);
    if (profile<0) {
        seamless_failure_reason=4;
        stop_cache(1,"Unexpected weapon asset; vanilla rendering retained.");return;
    }
    uint32_t offset=u32(raw+8);
    if (offset>(uint32_t)size-56 || memcmp((void *)(raw+offset),profile?heart_model:expected_model[index],56)) {
        stop_cache(1,"Unsupported weapon geometry header; vanilla rendering retained.");return;
    }
    uintptr_t resource=(uintptr_t)resource_for(data,0);
    if (!resource) {stop_cache(1,"Weapon file lacks native resource ownership; disabled.");return;}
    uintptr_t model=raw+offset;
    uint32_t effect_offset=u32(raw+4);
    if (!effect_layout(raw,(uint32_t)size,effect_offset,offset)) {
        stop_cache(1,"Unsupported cosmetic effect layout; disabled.");return;
    }
    /* Vanilla WPN completion calls this same initializer after the native
       file loader has registered PC mesh/texture metadata for its buffer. */
    init_model((void *)model);
    init_effect((void *)(raw+effect_offset));
    uintptr_t draw=(uintptr_t)resolve(u32(model+0x20));
    cached[index]=(struct Cached){raw,model,draw,resource,(uint32_t)size,index,raw+effect_offset,profile};
    if (!cached_valid(&cached[index])) {
        stop_cache(1,"PC graphics registration incomplete; vanilla rendering retained.");return;
    }
    seamless_ready_mask|=1u<<index;loading_index=-1;
    ready_if_complete();
}
static void begin_load(int index) {
    loading_index=index;seamless_phase=LOADING;load_started=GetTickCount64();
    ++seamless_loads;seamless_preload_step=(uint32_t)index+1;
    char message[128];snprintf(message,sizeof(message),"Preload %d/36: %s %s.",index+1,
        appearance_names[index%MODEL_COUNT],index<MODEL_COUNT?"model/effects":"sounds");report(message);
    /* NULL destination asks the native archive loader to allocate/register
       storage; no fixed-size external buffer and no equipped-weapon reload. */
    request_id=load_file(index<MODEL_COUNT?asset_names[index]:sound_names[index-MODEL_COUNT],NULL,loaded,0);
}
static int cosmetic_eligible(uintptr_t a) {
    return seamless_phase==READY && selected>=0 && selected<MODEL_COUNT && a==owner_actor &&
        context_valid() && gameplay() && standard_equipment() && attached(a) && cached_valid(&cached[selected]);
}
static int render_equipment(uintptr_t a) {
    /* Chest/event animations hide HUD and set cutscene flags even while the
       engine submits Sora's normal attached blade. Those are input gates,
       not a reason to revert its visible model. Require an actual standard
       Sora weapon parameter row so special event weapons still pass through. */
    if (!standard_equipment() || !attached(a) || *(unsigned char *)(game+0x5075A8)) return 0;
    uintptr_t params=ptr(weapon(a)+0x20),table=game+0x2D22D40+u32(game+0x528060);
    if (!readable(table,22*0x58) || params<table || params-table>=22*0x58 || (params-table)%0x58) return 0;
    unsigned row=(unsigned)((params-table)/0x58);
    return row==0 || (row>=5 && row<=21);
}
static void *__fastcall on_sound(int id,int tag,void *actor) {
    uintptr_t a=(uintptr_t)actor;
    if (cosmetic_eligible(a) && sound_source_valid(&sounds[selected])) {
        uintptr_t w=weapon(a),params=ptr(w+0x20);
        if (readable(params,0x58)) {
            uint32_t base=u32(params+0x34),variant=(uint32_t)id-base;
            /* Preserve caller's native hit variant and playback tag. These
               two hooks are only the weapon sound-base call sites. */
            if (base && variant<35 && sound_available(selected,variant)) {
                id=(int)(cosmetic_sound_base[selected]+variant);++seamless_sound_overrides;
            }
        }
    }
    return original_sound(id,tag);
}
static void *__fastcall on_trail(void *actor,void *bank,int index) {
    uintptr_t a=(uintptr_t)actor;
    if (cosmetic_eligible(a)) {
        uintptr_t w=weapon(a),effects=cached[selected].effects;
        if ((uintptr_t)bank==ptr(w+0x18)) {
            unsigned id=(unsigned)index&0x7FF,count=u32(effects+12);
            if (count==9 && within(effects,16+count*32,cached[selected].raw,cached[selected].bytes)) {
                for (unsigned i=0;i<count;++i) if (u32(effects+16+i*32+8)==id &&
                    within((uintptr_t)resolve(u32(effects+16+i*32)),16,cached[selected].raw,cached[selected].bytes)) {
                    bank=(void *)effects;++seamless_trail_overrides;break;
                }
            }
        }
    }
    return original_trail(actor,bank,index);
}
static uint64_t __fastcall on_submit(void *model,void *matrices,void *packet,void *actor) {
    uintptr_t p=(uintptr_t)packet,a=(uintptr_t)actor;
    /* During scene recovery the HUD/gameplay gate can lag visible rendering.
       Use only the currently published actor, never the old bound actor, and
       validate its live attachment plus the borrowed cosmetic on each draw.
       Input/audio/effects still wait for settled gameplay in tick_inputs. */
    if ((seamless_phase!=READY && seamless_phase!=SUSPENDED) || selected<0 || selected>=MODEL_COUNT ||
        a!=ptr(game+0x2D37280) || !readable(a,0x4B0) || (u32(a+0x374)&3)!=1 ||
        !*(unsigned char *)(game+0x2D5CC4C) ||
        !render_equipment(a) || !readable(p,0x60) ||
        !ptr(p+0x18)) return original_submit(model,matrices,packet,actor);
    if (!cached_valid(&cached[selected])) {
        stop_cache(0,"Cached asset unloaded; vanilla rendering until restart.");
        return original_submit(model,matrices,packet,actor);
    }
    /* The renderer copies this pointer into its draw records. Keep cached
       resources alive, and leave the actual actor/weapon/parameter table alone. */
    uintptr_t old=ptr(p+0x18);uint32_t handle=u32(a+0x14C);
    *(uintptr_t *)(p+0x18)=cached[selected].draw;
    ++seamless_render_overrides;
    uint64_t result=original_submit(model,matrices,packet,actor);
    *(uintptr_t *)(p+0x18)=old;
    if (u32(a+0x14C)!=handle) ++seamless_unexpected_weapon_changes;
    return result;
}
static void tick_inputs(int down,int reset,int focused,ULONGLONG now) {
    ++seamless_frame_count;
    int edge=down&&!was_q;was_q=down;
    if (seamless_phase==INVALIDATED || seamless_phase==FAILED) return;
    uintptr_t a=ptr(game+0x2D37280);
    if (seamless_phase==READY && !context_valid()) {
        seamless_phase=SUSPENDED;idle_tracking=0;
        report("Scene changing; cosmetic selection retained while native rendering continues.");return;
    }
    if (seamless_phase==LOADING && !context_valid()) {
        stop_cache(0,"Scene changed during initial preload; restart to retry.");return;
    }
    if (seamless_phase==SUSPENDED) {
        /* Never preload again or dereference old actor pointers. Bind only
           after the new scene has an attached standard weapon and is stable.
           Every borrowed resource must still pass registry/header checks. */
        int settled=gameplay() && standard_equipment() && attached(a) &&
            !u32(game+0x2867370);
        if (!settled) {idle_tracking=0;return;}
        uint32_t model=u32(a+0x130);
        if (!idle_tracking || a!=owner_actor || model!=owner_model) {
            owner_actor=a;owner_model=model;idle_since=now;idle_tracking=1;return;
        }
        if (now-idle_since<500) return;
        for (int i=0;i<MODEL_COUNT;++i) if (!cached_valid(&cached[i]) || !sound_valid(&sounds[i])) {
            stop_cache(0,"Scene unloaded cosmetic resources; vanilla rendering until restart.");return;
        }
        seamless_phase=READY;idle_tracking=0;++seamless_scene_resumes;
        report("New scene ready; retained cosmetic appearance restored.");return;
    }
    if (seamless_phase==WAITING) {
        int idle=gameplay() && standard_equipment() && attached(a) &&
            !u32(a+0x164) && !u32(a+0x70) && !u32(game+0x2867370);
        if (!idle) {idle_tracking=0;return;}
        if (!idle_tracking) {idle_tracking=1;idle_since=now;return;}
        if (now-idle_since<500) return;
        owner_actor=a;owner_model=u32(a+0x130);report("Preloading 18 cosmetic models/effect banks and 18 sound banks; equipped weapon stays attached.");
        begin_load(0);return;
    }
    if (seamless_phase==LOADING) {
        if (loading_index<0) {
            for (int i=0;i<MODEL_COUNT;++i) if (!(seamless_ready_mask&(1u<<i))) {begin_load(i);return;}
            for (int i=0;i<MODEL_COUNT;++i) if (!(seamless_sound_ready_mask&(1u<<i))) {begin_load(MODEL_COUNT+i);return;}
            ready_if_complete();return;
        }
        if (now-load_started>10000) stop_cache(1,"Preload timed out; vanilla gameplay continues. Restart to retry.");
        return;
    }
    if (!edge || !focused || !gameplay() ||
        !standard_equipment() || !attached(a) || u32(game+0x2867370)) return;
    /* Once preloaded, swings do not require a reload or an idle gate. */
    selected=reset?-1:(selected+1)%MODEL_COUNT;
    ++seamless_swaps;
    if (selected<0) report("Equipped appearance.");
    else {char message[96];snprintf(message,sizeof(message),"Appearance/trails/hit sounds: %s.",appearance_names[selected]);report(message);}
}
static uint64_t __cdecl on_frame(void *context) {
    DWORD foreground=0;GetWindowThreadProcessId(GetForegroundWindow(),&foreground);
    tick_inputs((GetAsyncKeyState('Q')&0x8000)!=0,(GetAsyncKeyState(VK_SHIFT)&0x8000)!=0,
        foreground==GetCurrentProcessId(),GetTickCount64());
    return original_frame(context);
}
static const uintptr_t effect_sites[4]={0x2BB3E9,0x2C2015,0x2954F2,0x29619E};
static const unsigned char effect_calls[4][5]={
    {0xE8,0xA2,0x2E,0xFF,0xFF},{0xE8,0x76,0xC2,0xFE,0xFF},
    {0xE8,0xF9,0x0C,0,0},{0xE8,0x4D,0,0,0}
};
static int supported(void) {
    static const unsigned char load_sig[]={0x48,0x89,0x5C,0x24,0x20,0x55,0x56,0x41,0x54,0x48};
    static const unsigned char init_sig[]={0xE9,0xDB,0x14,0,0};
    static const unsigned char resource_sig[]={0x4C,0x8B,0x1D,0xF9,0x84,0x0C,0x02};
    static const unsigned char scheduler_sig[]={0xE8,0x52,0x60,0,0};
    for (int i=0;i<4;++i) if (!bytes_at(effect_sites[i],effect_calls[i],5)) return 0;
    static const unsigned char effect_sig[]={0x48,0x89,0x5C,0x24,8,0x48,0x89,0x74,0x24,0x10};
    static const unsigned char bank_sig[]={0x48,0x89,0x5C,0x24,0x10};
    static const unsigned char lookup_sig[]={0x48,0x89,0x6C,0x24,0x10};
    return bytes_at(0x1E0080,effect_sig,sizeof(effect_sig)) &&
        bytes_at(0x296040,bank_sig,sizeof(bank_sig)) && bytes_at(0x2961F0,lookup_sig,sizeof(lookup_sig)) &&
        *(unsigned char *)(game+0x4698D2)==106 && u32(game+0x3EA388)==540680280 &&
        bytes_at(SUBMIT_CALL,submit_bytes,5) && bytes_at(0x28BC10,load_sig,sizeof(load_sig)) &&
        bytes_at(0x1D6750,init_sig,5) && bytes_at(0xE2B20,resource_sig,7) &&
        bytes_at(0x2846B9,scheduler_sig,5); /* Refuse concurrent stable helper. */
}
static int install_render_hook(void) {
    uintptr_t sites[5]={SUBMIT_CALL,effect_sites[0],effect_sites[1],effect_sites[2],effect_sites[3]};
    uintptr_t callbacks[5]={(uintptr_t)&on_submit,(uintptr_t)&on_trail,(uintptr_t)&on_trail,(uintptr_t)&on_sound,(uintptr_t)&on_sound};
    unsigned char patches[5][5];DWORD protection[5],unused;int protected_count=0;
    if (!bytes_at(SUBMIT_CALL,submit_bytes,5)) return 0;
    for (int i=0;i<4;++i) if (!bytes_at(effect_sites[i],effect_calls[i],5)) return 0;
    SYSTEM_INFO si;GetSystemInfo(&si);uintptr_t gran=si.dwAllocationGranularity;
    uintptr_t anchor=(game+SUBMIT_CALL)&~(gran-1);
    for (uintptr_t d=gran;d<0x7FFF0000u && !relay;d+=gran) {
        if (anchor>d) relay=VirtualAlloc((void *)(anchor-d),gran,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);
        if (!relay && anchor+d>anchor) relay=VirtualAlloc((void *)(anchor+d),gran,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);
    }
    if (!relay) return 0;
    for (int i=0;i<5;++i) {
        unsigned char *entry=(unsigned char *)relay+i*32,*jump=entry;
        /* At the sound-base calls RDI/RSI holds the originating actor.
           R8 is volatile and not an argument to the original two-arg lookup.
           Supply it to our three-arg callback without touching nonvolatiles. */
        if (i>=3) {entry[0]=0x49;entry[1]=0x89;entry[2]=i==3?0xF8:0xF0;jump+=3;}
        jump[0]=0x48;jump[1]=0xB8;memcpy(jump+2,&callbacks[i],8);jump[10]=0xFF;jump[11]=0xE0;
        int64_t distance=(int64_t)(uintptr_t)entry-(int64_t)(game+sites[i]+5);
        if (distance<INT32_MIN || distance>INT32_MAX) goto fail;
        int32_t rel=(int32_t)distance;patches[i][0]=0xE8;memcpy(patches[i]+1,&rel,4);
        if (i) extra_relays[i-1]=entry;
    }
    if (!VirtualProtect(relay,gran,PAGE_EXECUTE_READ,&unused)) goto fail;
    FlushInstructionCache(GetCurrentProcess(),relay,160);
    /* Acquire all writable sites before committing any patch. A failed
       allocation/signature/protection leaves every original call intact. */
    for (int i=0;i<5;++i) {
        if (!VirtualProtect((void *)(game+sites[i]),5,PAGE_EXECUTE_READWRITE,&protection[i])) goto fail;
        ++protected_count;
    }
    original_submit=(SubmitProc)(game+0x1D6510);
    original_sound=(SoundProc)(game+0x2961F0);original_trail=(TrailProc)(game+0x2AE290);
    for (int i=0;i<5;++i) memcpy((void *)(game+sites[i]),patches[i],5);
    for (int i=4;i>=0;--i) {
        FlushInstructionCache(GetCurrentProcess(),(void *)(game+sites[i]),5);
        VirtualProtect((void *)(game+sites[i]),5,protection[i],&unused);
    }
    return 1;
fail:
    for (int i=protected_count-1;i>=0;--i)
        VirtualProtect((void *)(game+sites[i]),5,protection[i],&unused);
    VirtualFree(relay,0,MEM_RELEASE);relay=NULL;memset(extra_relays,0,sizeof(extra_relays));return 0;
}
__declspec(dllexport) int __cdecl kh1_transmog_bootstrap(void *lua_state) {
    (void)lua_state;if(enabled||failed)return 0;
    game=(uintptr_t)GetModuleHandleW(NULL);
    if(!supported()){failed=1;report("Unsupported executable or conflicting helper; disabled.");return 0;}
    resolve=(ResolveProc)(game+0x38ADC0);load_file=(LoadProc)(game+0x28BC10);
    init_model=(InitProc)(game+0x1D6750);init_effect=(InitProc)(game+0x1E0080);
    install_bank=(BankProc)(game+0x296040);resource_for=(ResourceProc)(game+0xE2B20);
    uintptr_t app=ptr(game+0x21AAF18);
    if(!readable(app,8)||!readable(ptr(app),40))return 0;
    FrameProc *slot=(FrameProc *)(ptr(app)+32);if(!*slot)return 0;
    DWORD old;if(!VirtualProtect(slot,8,PAGE_READWRITE,&old)){failed=1;return 0;}
    HMODULE pinned;
    if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_PIN,
        (LPCWSTR)(uintptr_t)&on_frame,&pinned)||!install_render_hook()){
        DWORD unused;VirtualProtect(slot,8,old,&unused);failed=1;report("Hook installation failed; disabled.");return 0;
    }
    original_frame=(FrameProc)InterlockedExchangePointer((PVOID volatile *)slot,(PVOID)&on_frame);
    DWORD unused;VirtualProtect(slot,8,old,&unused);enabled=1;
    report("Experimental 0.2.0-p8 graphics/effect/sound hooks installed. Waiting for idle gameplay.");return 0;
}
