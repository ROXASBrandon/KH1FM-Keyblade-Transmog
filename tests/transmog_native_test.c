/* Windows-only native integration test. Uses isolated memory; never opens or
   patches a running game. Exercises capture/apply/restore and conflict guards. */
#undef NDEBUG
#include <assert.h>
#include "kh1_transmog.c"

int main(void) {
    game=(uintptr_t)VirtualAlloc(NULL,0x4000000,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);
    assert(game);
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
    puts("PASS: Windows native capture/apply/reset, all 324 model/sound pairs, combat preservation and conflict guards.");
    VirtualFree((void *)game,0,MEM_RELEASE);
    return 0;
}
