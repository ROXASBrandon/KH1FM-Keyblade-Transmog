#undef NDEBUG
#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include "transmog_core.h"
int main(void) {
    unsigned char table[22*TM_STRIDE], before[sizeof(table)], model[TM_MODEL_SIZE];
    for (size_t i=0;i<sizeof(table);++i) table[i]=(unsigned char)(i*37+11);
    memcpy(before,table,sizeof(table));
    unsigned char original_names[TM_COUNT][TM_MODEL_SIZE];
    for (int i=0;i<TM_COUNT;++i) {
        memset(original_names[i],0,TM_MODEL_SIZE);
        memcpy(original_names[i],tm_models[i],strlen(tm_models[i]));
    }
    for (int target=0;target<TM_COUNT;++target) {
        memset(model,0,sizeof(model));memcpy(model,tm_models[target],strlen(tm_models[target]));
        for (int i=0;i<TM_COUNT;++i) tm_copy_model(table+tm_rows[i]*TM_STRIDE,model);
        for (int row=0;row<22;++row) {
            int i=tm_index_for_row(row);
            assert(!memcmp(table+row*TM_STRIDE+TM_MODEL_SIZE,before+row*TM_STRIDE+TM_MODEL_SIZE,TM_STRIDE-TM_MODEL_SIZE));
            if (i<0) assert(!memcmp(table+row*TM_STRIDE,before+row*TM_STRIDE,TM_STRIDE));
            else assert(!memcmp(table+row*TM_STRIDE,model,TM_MODEL_SIZE));
            const char *name=(const char *)(table+row*TM_STRIDE);
            const char *sound=tm_sound_model(name,table,original_names,1);
            if (i<0) assert(sound==name);
            else assert(sound==(const char *)original_names[i]);
            assert(tm_sound_model(name,table,original_names,0)==name);
        }
        for (int i=0;i<TM_COUNT;++i) tm_copy_model(table+tm_rows[i]*TM_STRIDE,before+tm_rows[i]*TM_STRIDE);
        assert(!memcmp(table,before,sizeof(table)));
    }
    for (int i=0;i<TM_COUNT;++i) {
        int next=tm_next(-1,tm_rows[i]);assert(next==(i+1)%TM_COUNT);
        int seen=0;
        for(int n=0;n<TM_COUNT;++n) { seen|=1<<next;next=tm_next(next,tm_rows[i]); }
        assert(seen==(1<<TM_COUNT)-1);
    }
    int held=0;
    assert(tm_key_edge(1,1,&held));
    for(int i=0;i<120;++i)assert(!tm_key_edge(1,1,&held));
    assert(!tm_key_edge(0,1,&held));assert(tm_key_edge(1,1,&held));
    assert(!tm_key_edge(0,0,&held));assert(!tm_key_edge(1,0,&held));
    assert(!tm_key_edge(1,1,&held));assert(!tm_key_edge(0,1,&held));
    assert(tm_key_edge(1,1,&held));
    assert(tm_index_for_row(1)==-1 && tm_index_for_row(4)==-1 && tm_index_for_row(22)==-1);
    const char *unrelated="other_character";
    assert(tm_sound_model(unrelated,table,original_names,1)==unrelated);
    puts("PASS: all 18 appearances, equipped sound-name isolation, stats/sound IDs/reach/critical bytes unchanged, exact restoration, cycle wrap, held Q, out-of-gameplay input.");
}
