#ifndef TRANSMOG_CORE_H
#define TRANSMOG_CORE_H
#include <stddef.h>
#include <string.h>
/* Record indices in Sora's 0x58-byte weapon table. Stats begin at +0x20. */
static const int tm_rows[] = {0,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21};
static const char *const tm_names[] = {
    "Kingdom Key", "Jungle King", "Three Wishes", "Fairy Harp", "Pumpkinhead",
    "Crabclaw", "Divine Rose", "Spellbinder", "Olympia", "Lionheart",
    "Metal Chocobo", "Oathkeeper", "Oblivion", "Lady Luck", "Wishing Star",
    "Ultima Weapon", "Diamond Dust", "One-Winged Angel"
};
static const char *const tm_models[] = {
    "xw_ex_5010", "xw_ex_5020", "xw_ex_5030", "xw_ex_5040", "xw_ex_5050",
    "xw_ex_5060", "xw_ex_5070", "xw_ex_5080", "xw_ex_5090", "xw_ex_5100",
    "xw_ex_5110", "xw_ex_5120", "xw_ex_5130", "xw_ex_5140", "xw_ex_5150",
    "xw_ex_5160", "xw_ex_5190", "xw_ex_5200"
};
#define TM_COUNT 18
#define TM_STRIDE 0x58
#define TM_MODEL_SIZE 0x20
static int tm_index_for_row(int row) {
    for (int i = 0; i < TM_COUNT; ++i) if (tm_rows[i] == row) return i;
    return -1;
}
static int tm_next(int selected, int equipped_row) {
    int start = selected >= 0 ? selected : tm_index_for_row(equipped_row);
    return (start + 1) % TM_COUNT;
}
/* A held key and a press made outside gameplay never cause a delayed swap. */
static int tm_key_edge(int down, int eligible, int *was_down) {
    int edge = down && !*was_down;
    *was_down = down;
    return edge && eligible;
}
static void tm_copy_model(unsigned char *record, const unsigned char *model) {
    memcpy(record, model, TM_MODEL_SIZE);
}
/* Sound IDs remain equipped. Both vanilla .se loaders must use that row's
   original name even while its .wpn name is overridden. */
static const char *tm_sound_model(const char *model, const unsigned char *table_base,
                                 const unsigned char originals[TM_COUNT][TM_MODEL_SIZE], int captured) {
    if (captured)
        for (int i=0;i<TM_COUNT;++i)
            if (model==(const char *)(table_base+tm_rows[i]*TM_STRIDE))
                return (const char *)originals[i];
    return model;
}
#endif
