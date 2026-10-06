/* Windows-only native integration test. Uses an isolated fake image, never
   opens or patches a running game. Exercises actual rel32 relay and Win64 ABI. */
#undef NDEBUG
#include <assert.h>
#include "kh1_transmog.c"

static int __cdecl fake_format(char *dest, const char *format,
                              const char *model, const char *extension) {
    return snprintf(dest,160,format,model,extension);
}
static void jump_to(unsigned char *dest, uintptr_t function) {
    dest[0]=0x48;dest[1]=0xB8;memcpy(dest+2,&function,8);
    dest[10]=0xFF;dest[11]=0xE0;
}
int main(void) {
    game=(uintptr_t)VirtualAlloc(NULL,0x4000000,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);
    assert(game);
    table=game+0x1000000;captured=1;
    for (int i=0;i<TM_COUNT;++i)
        memcpy(originals[i],tm_models[i],strlen(tm_models[i])+1);
    jump_to((unsigned char *)(game+0x51260),(uintptr_t)&fake_format);
    uintptr_t sites[]={game+0x286CCC,game+0x2871DA};
    for (int i=0;i<2;++i) {
        unsigned char *site=(unsigned char *)sites[i];
        const unsigned char enter[]={0x48,0x83,0xEC,0x28};
        const unsigned char leave[]={0x48,0x83,0xC4,0x28,0xC3};
        memcpy(site-4,enter,4);site[0]=0xE8;
        int32_t displacement=(int32_t)(game+0x51260-(sites[i]+5));
        memcpy(site+1,&displacement,4);memcpy(site+5,leave,5);
    }
    DWORD ignored;
    assert(VirtualProtect((void *)(game+0x51000),0x1000,PAGE_EXECUTE_READ,&ignored));
    assert(VirtualProtect((void *)(game+0x286000),0x2000,PAGE_EXECUTE_READ,&ignored));
    assert(install_sound_hooks());
    char actual[160],expected[160];
    for (int target=0;target<TM_COUNT;++target) {
        for (int equipped=0;equipped<TM_COUNT;++equipped) {
            char *name=(char *)(table+tm_rows[equipped]*TM_STRIDE);
            tm_copy_model((unsigned char *)name,originals[target]);
            snprintf(expected,sizeof(expected),"%s.se",tm_models[equipped]);
            for (int i=0;i<2;++i) {
                ((FormatProc)(sites[i]-4))(actual,"%s%s",name,".se");
                assert(!strcmp(actual,expected));
            }
            ((FormatProc)(game+0x51260))(actual,"%s%s",name,".wpn");
            snprintf(expected,sizeof(expected),"%s.wpn",tm_models[target]);
            assert(!strcmp(actual,expected));
        }
    }
    captured=0;
    ((FormatProc)(sites[0]-4))(actual,"%s%s","other_character",".se");
    assert(!strcmp(actual,"other_character.se"));
    puts("PASS: Windows relay/ABI, both sound loaders, all 324 equipped/appearance pairs, graphics and unrelated names unchanged.");
    VirtualFree((void *)game,0,MEM_RELEASE);
    return 0;
}
