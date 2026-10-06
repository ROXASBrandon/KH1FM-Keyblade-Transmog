#ifndef TRANSMOG_CORE_H
#define TRANSMOG_CORE_H
#include <stddef.h>
#include <string.h>
#include <stdint.h>
/* Record indices in Sora's 0x58-byte weapon table. Combat fields begin at
   +0x20, interleaved with a four-byte cosmetic sound ID at +0x34. */
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
#define TM_SOUND_OFFSET 0x34
#define TM_SOUND_SIZE 4
#define TM_COSMETIC_SIZE (TM_MODEL_SIZE+TM_SOUND_SIZE)
#define TM_IDLE_MS 300
/* Animation ID is a DWORD: state 0 alone also includes swings and combos. */
static int tm_animation_idle(uint32_t animation) { return animation==0; }
static int tm_idle_gate(int idle, uint64_t now, uint64_t *since, int *tracking) {
    if (!idle) { *tracking=0;return 0; }
    if (!*tracking) { *since=now;*tracking=1;return 0; }
    return now-*since>=TM_IDLE_MS;
}
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
/* Copy only cosmetic fields; never copy the intervening combat parameters. */
static void tm_capture_cosmetic(unsigned char *cosmetic, const unsigned char *record) {
    memcpy(cosmetic,record,TM_MODEL_SIZE);
    memcpy(cosmetic+TM_MODEL_SIZE,record+TM_SOUND_OFFSET,TM_SOUND_SIZE);
}
static void tm_copy_cosmetic(unsigned char *record, const unsigned char *cosmetic) {
    memcpy(record,cosmetic,TM_MODEL_SIZE);
    memcpy(record+TM_SOUND_OFFSET,cosmetic+TM_MODEL_SIZE,TM_SOUND_SIZE);
}
static int tm_cosmetic_matches(const unsigned char *record, const unsigned char *cosmetic) {
    return !memcmp(record,cosmetic,TM_MODEL_SIZE)
        && !memcmp(record+TM_SOUND_OFFSET,cosmetic+TM_MODEL_SIZE,TM_SOUND_SIZE);
}
#endif
