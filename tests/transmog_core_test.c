#undef NDEBUG
#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include "transmog_core.h"
int main(void) {
    unsigned char table[22*TM_STRIDE], before[sizeof(table)];
    unsigned char originals[TM_COUNT][TM_COSMETIC_SIZE];
    for (size_t i=0;i<sizeof(table);++i) table[i]=(unsigned char)(i*37+11);
    memcpy(before,table,sizeof(table));
    for (int i=0;i<TM_COUNT;++i)
        tm_capture_cosmetic(originals[i],before+tm_rows[i]*TM_STRIDE);
    for (int target=0;target<TM_COUNT;++target) {
        for (int i=0;i<TM_COUNT;++i)
            tm_copy_cosmetic(table+tm_rows[i]*TM_STRIDE,originals[target]);
        for (int row=0;row<22;++row) {
            int i=tm_index_for_row(row);
            unsigned char *record=table+row*TM_STRIDE;
            unsigned char *original=before+row*TM_STRIDE;
            for (int byte=0;byte<TM_STRIDE;++byte) {
                int cosmetic=byte<TM_MODEL_SIZE || (byte>=TM_SOUND_OFFSET && byte<TM_SOUND_OFFSET+TM_SOUND_SIZE);
                if (i<0 || !cosmetic) assert(record[byte]==original[byte]);
            }
            if (i>=0) assert(tm_cosmetic_matches(record,originals[target]));
        }
        for (int i=0;i<TM_COUNT;++i)
            tm_copy_cosmetic(table+tm_rows[i]*TM_STRIDE,originals[i]);
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
    puts("PASS: all 324 model/sound combinations, every combat byte unchanged, exact restoration, cycle wrap, held Q, out-of-gameplay input.");
}
