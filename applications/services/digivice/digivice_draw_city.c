#include "digivice_draw.h"
#include "digivice_city.h"

// Interim: Task 10 replaces this attribute mapping with the real sprites.
bool digivice_draw_city_monster(
    Canvas* canvas,
    uint8_t idx,
    int32_t cx,
    int32_t foot_y,
    uint32_t frame,
    DigivicePose pose,
    bool flip,
    bool silhouette) {
    if(idx >= DIGIVICE_CITY_COUNT) return false;
    static const uint8_t by_attr[DigiviceAttrCount] = {
        DigiviceFoeKorno, DigiviceFoeFuzzcan, DigiviceFoeChipko, DigiviceFoeMokobit};
    digivice_draw_foe(
        canvas,
        (DigiviceFoe)by_attr[digivice_city_monster_attr(idx)],
        cx,
        foot_y,
        frame,
        pose,
        flip,
        silhouette);
    return true;
}
