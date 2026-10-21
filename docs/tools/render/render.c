// Host renderer: draws the REAL digivice screens with the firmware's own drawing code.
//
// This file #includes digivice_app.c and digivice_app_battle.c so it can call their static draw
// and tick functions; every other digivice source is linked as-is. Only the Flipper SDK (furi,
// gui, notification) is replaced by the shims in ./shim and host_canvas.c, which backs the
// canvas with the real u8g2 library. Output: raw 128x64 frames, one file per scene, that
// to_png.py turns into PNGs and GIFs. Usage: render <out-dir>
#include "digivice_app.c"
#include "digivice_app_battle.c"

#include <digivice/digivice_notify.h>

#define DAY(h) (20740UL * 86400UL + (h) * 3600UL)

// ---- host stand-ins for the SDK and the service ----------------------------------------------

static uint32_t g_now = DAY(14);
static uint32_t g_dice = 0x2468ACE1;
static DigiviceState S; // what the service would hold; the app reads it through digivice_get_state

uint32_t furi_hal_random_get(void) {
    g_dice ^= g_dice << 13;
    g_dice ^= g_dice >> 17;
    g_dice ^= g_dice << 5;
    return g_dice;
}
uint32_t furi_hal_rtc_get_timestamp(void) {
    return g_now;
}
uint32_t furi_get_tick(void) {
    return 0;
}
uint32_t furi_kernel_get_tick_frequency(void) {
    return 1000;
}
uint32_t furi_ms_to_ticks(uint32_t ms) {
    return ms;
}
FuriMutex* furi_mutex_alloc(FuriMutexType t) {
    UNUSED(t);
    return (FuriMutex*)malloc(1);
}
FuriStatus furi_mutex_acquire(FuriMutex* m, uint32_t t) {
    UNUSED(m);
    UNUSED(t);
    return FuriStatusOk;
}
FuriStatus furi_mutex_release(FuriMutex* m) {
    UNUSED(m);
    return FuriStatusOk;
}
void furi_mutex_free(FuriMutex* m) {
    free(m);
}
FuriMessageQueue* furi_message_queue_alloc(uint32_t c, uint32_t s) {
    UNUSED(c);
    UNUSED(s);
    return NULL;
}
FuriStatus furi_message_queue_put(FuriMessageQueue* q, const void* m, uint32_t t) {
    UNUSED(q);
    UNUSED(m);
    UNUSED(t);
    return FuriStatusOk;
}
FuriStatus furi_message_queue_get(FuriMessageQueue* q, void* m, uint32_t t) {
    UNUSED(q);
    UNUSED(m);
    UNUSED(t);
    return FuriStatusError;
}
void furi_message_queue_free(FuriMessageQueue* q) {
    UNUSED(q);
}
void* furi_record_open(const char* n) {
    UNUSED(n);
    return NULL;
}
void furi_record_close(const char* n) {
    UNUSED(n);
}
bool furi_record_exists(const char* n) {
    UNUSED(n);
    return false;
}
void notification_message(NotificationApp* a, const NotificationSequence* s) {
    UNUSED(a);
    UNUSED(s);
}
ViewPort* view_port_alloc(void) {
    return NULL;
}
void view_port_free(ViewPort* v) {
    UNUSED(v);
}
void view_port_draw_callback_set(ViewPort* v, void (*cb)(Canvas*, void*), void* c) {
    UNUSED(v);
    UNUSED(cb);
    UNUSED(c);
}
void view_port_input_callback_set(ViewPort* v, void (*cb)(InputEvent*, void*), void* c) {
    UNUSED(v);
    UNUSED(cb);
    UNUSED(c);
}
void view_port_update(ViewPort* v) {
    UNUSED(v);
}
void gui_add_view_port(Gui* g, ViewPort* v, GuiLayer l) {
    UNUSED(g);
    UNUSED(v);
    UNUSED(l);
}
void gui_remove_view_port(Gui* g, ViewPort* v) {
    UNUSED(g);
    UNUSED(v);
}

#define SEQ(n) const NotificationSequence n = {0}
SEQ(sequence_set_only_blue_255);
SEQ(sequence_set_only_red_255);
SEQ(sequence_set_only_green_255);
SEQ(sequence_blink_white_100);
SEQ(sequence_blink_green_100);
SEQ(sequence_reset_rgb);
SEQ(digivice_sequence_virux);
SEQ(digivice_sequence_evolution);
SEQ(digivice_sequence_evolution_silent);
SEQ(digivice_sequence_charge);
SEQ(digivice_sequence_hit);
SEQ(digivice_sequence_win);
SEQ(digivice_sequence_sick);
SEQ(digivice_sequence_sick_silent);
SEQ(digivice_sequence_poop);
SEQ(digivice_sequence_bedtime);
SEQ(digivice_sequence_encounter);
SEQ(digivice_sequence_special);
SEQ(digivice_sequence_special_hold);
SEQ(digivice_sequence_lose);
SEQ(digivice_sequence_call);

// The service API the app calls. Only these two matter for drawing; the rest are never reached.
void digivice_get_state(Digivice* d, DigiviceState* out) {
    UNUSED(d);
    *out = S;
}
DigiviceReward digivice_battle_finish(
    Digivice* d,
    DigiviceEncounterKind kind,
    DigiviceBattleResult result,
    bool shield_used,
    uint32_t offered_expires) {
    UNUSED(d);
    return digivice_rules_battle_result(
        &S, kind, result, shield_used, offered_expires, (uint8_t)(furi_hal_random_get() % 100), g_now);
}
bool digivice_feed(Digivice* d, DigiviceFood f) {
    UNUSED(d);
    return digivice_rules_feed(&S, f, g_now);
}
DigiviceResult digivice_train(Digivice* d, bool ok) {
    UNUSED(d);
    return digivice_rules_train(&S, ok, g_now);
}
void digivice_ack_evolution(Digivice* d) {
    UNUSED(d);
}
void digivice_set_companion(Digivice* d, uint8_t id) {
    UNUSED(d);
    UNUSED(id);
}
DigiviceResult digivice_clean(Digivice* d) {
    UNUSED(d);
    return DigiviceResultOk;
}
DigiviceResult digivice_cure(Digivice* d) {
    UNUSED(d);
    return DigiviceResultOk;
}
DigiviceResult digivice_toggle_light(Digivice* d) {
    UNUSED(d);
    return DigiviceResultOk;
}
void digivice_set_lang(Digivice* d, DigiviceLang l) {
    UNUSED(d);
    UNUSED(l);
}
void digivice_set_mode(Digivice* d, DigiviceMode m) {
    UNUSED(d);
    UNUSED(m);
}
void digivice_set_city(Digivice* d, uint8_t c) {
    UNUSED(d);
    UNUSED(c);
}
void digivice_reset(Digivice* d) {
    UNUSED(d);
}
uint8_t digivice_hike_finish(Digivice* d, uint8_t g) {
    UNUSED(d);
    UNUSED(g);
    return 0;
}
bool digivice_select_area(Digivice* d, uint8_t a) {
    UNUSED(d);
    UNUSED(a);
    return true;
}
DigiviceMidwayResult digivice_midway_claim(Digivice* d, bool a) {
    UNUSED(d);
    UNUSED(a);
    DigiviceMidwayResult r = {DigiviceMidwayCount, false};
    return r;
}

// ---- scene plumbing --------------------------------------------------------------------------

static const char* out_dir;
static FILE* out;
static Canvas* cv;
static DigiviceApp* app;

static void begin(const char* name) {
    char path[512];
    snprintf(path, sizeof(path), "%s/%s.f", out_dir, name);
    out = fopen(path, "wb");
    if(!out) {
        perror(path);
        exit(1);
    }
}

static void end(void) {
    fclose(out);
    out = NULL;
}

static void emit(void) {
    uint8_t px[128 * 64];
    canvas_dump(cv, px);
    fwrite(px, 1, sizeof(px), out);
}

static void reset_canvas(void) {
    canvas_set_color(cv, ColorBlack);
    canvas_set_font(cv, FontSecondary);
}

/** One frame of the app exactly as the viewport would draw it. */
static void snap(void) {
    reset_canvas();
    digivice_app_draw_callback(cv, app);
    emit();
}

/** The app's own 100 ms tick: frame counters, then the screen's state machine. */
static void tick(void) {
    digivice_app_tick(app);
    app->state = S;
}

static DigiviceState pet(DigiviceForm form) {
    DigiviceState s;
    digivice_rules_init(&s, g_now - 3 * 86400UL);
    s.form = s.prev_form = (uint8_t)form;
    s.lang = DigiviceLangEn;
    s.hunger = 3;
    s.mood = 3;
    s.vigor = 3;
    s.strength = 14;
    s.last_tick = g_now;
    s.stage_since = g_now - 3600;
    s.exp = 120;
    s.wins = 12;
    s.losses = 3;
    return s;
}

static void new_app(const DigiviceState* s) {
    if(app) free(app->scene);
    free(app);
    app = calloc(1, sizeof(DigiviceApp));
    app->running = true;
    app->mutex = furi_mutex_alloc(FuriMutexTypeNormal);
    S = *s;
    app->state = S;
    app->frame = 7;
    digivice_app_set_screen(app, DigiviceScreenMain);
}

static void fight(DigiviceEncounterKind kind, uint8_t enc_foe) {
    S.enc_foe = enc_foe;
    S.enc_expires = g_now + 1200;
    app->state = S;
    digivice_encounter_start(app, kind);
}

static void to_menu(void) {
    digivice_app_set_screen(app, DigiviceScreenBattle);
    battle_set_phase(app, DigiviceBattlePhaseMenu);
}

/** Lead-in shots: `frames` are screen_frame values of the encounter intro. */
static void intro(const uint32_t* frames, size_t n) {
    for(size_t i = 0; i < n; i++) {
        app->screen_frame = frames[i];
        app->frame += 3;
        snap();
    }
}

// ---- scenes ----------------------------------------------------------------------------------

static void scene_home(void) {
    DigiviceState s = pet(DigiviceFormWyvolt);
    s.poop = 1;
    s.exp = 240;
    begin("anim-home-hud");
    for(uint32_t f = 0; f < 24; f++) {
        canvas_clear(cv);
        reset_canvas();
        digivice_draw_home(cv, &s, f, g_now, true);
        emit();
    }
    end();

    s = pet(DigiviceFormFuzzcan);
    s.hunger = 1;
    s.call_flags = DIGIVICE_CALL_HUNGER;
    begin("shot-home-hud-call");
    canvas_clear(cv);
    reset_canvas();
    digivice_draw_home(cv, &s, 5, g_now, true);
    emit();
    end();

    s = pet(DigiviceFormAriete);
    s.mode = DigiviceModeHacker;
    s.virux_type = DigiviceTypeSubGhz;
    s.virux_expires = g_now + 1500;
    begin("shot-home-hud-virux");
    canvas_clear(cv);
    reset_canvas();
    digivice_draw_home(cv, &s, 6, g_now, true);
    emit();
    end();

    // Night: asleep with the lights off.
    s = pet(DigiviceFormKorno);
    s.light_on = 0;
    uint32_t keep = g_now;
    g_now = DAY(2);
    begin("shot-home-hud-night");
    canvas_clear(cv);
    reset_canvas();
    digivice_draw_home(cv, &s, 6, g_now, true);
    emit();
    end();
    g_now = keep;
}

static void scene_menu(void) {
    DigiviceState s = pet(DigiviceFormFuzzcan);
    s.poop = 2;
    new_app(&s);
    app->menu_index = DigiviceIconBattle;
    digivice_app_notice(app, DigiviceStrMenuBattle);
    begin("shot-icon-menu");
    app->frame = 22;
    snap();
    end();

    new_app(&s);
    begin("anim-pet-walk");
    for(int i = 0; i < 40; i++) {
        snap();
        tick();
    }
    end();

    s = pet(DigiviceFormVampixel);
    s.sick = 1;
    s.poop = 3;
    s.call_flags = DIGIVICE_CALL_HUNGER;
    s.hunger = 1;
    new_app(&s);
    app->frame = 20;
    begin("shot-care-needs");
    snap();
    end();

    s = pet(DigiviceFormChispi);
    s.corruption = 70;
    new_app(&s);
    app->frame = 24;
    begin("shot-corruption-glitch");
    snap();
    end();
}

static void scene_feed(void) {
    DigiviceState s = pet(DigiviceFormEstatik);
    s.hunger = 1;
    s.vigor = 2;
    new_app(&s);
    digivice_feed_enter(app);
    begin("shot-feeding-pick");
    snap();
    end();

    // Meat is eaten: the real handler runs against the stub service.
    app->feed_food = DigiviceFoodMeat;
    digivice_feed_input(app, &(InputEvent){.key = InputKeyOk, .type = InputTypeShort});
    begin("anim-feeding");
    for(int i = 0; i < DIGIVICE_FEED_FRAMES - 1; i++) {
        snap();
        app->frame++;
        app->screen_frame++;
    }
    end();
    app->screen_frame = 40;
    begin("shot-feeding-hearts");
    snap();
    end();
}

static void scene_train_profile(void) {
    DigiviceState s = pet(DigiviceFormKorno);
    new_app(&s);
    digivice_train_enter(app);
    app->meter.zone_start = 11;
    app->meter.cursor = 12;
    begin("shot-training");
    snap();
    end();

    new_app(&s);
    digivice_app_set_screen(app, DigiviceScreenProfile);
    begin("shot-profile");
    snap();
    end();
}

static void scene_trail(void) {
    DigiviceState s = pet(DigiviceFormFuzzcan);
    s.area = 0;
    s.distance = 330;
    s.item = DigiviceItemShield;
    new_app(&s);
    digivice_app_set_screen(app, DigiviceScreenExpedition);
    app->trail_scroll = 40;
    begin("shot-trail-area1");
    snap();
    end();

    begin("anim-trail-walk");
    for(int i = 0; i < 48; i++) {
        snap();
        app->trail_scroll++;
        app->frame++;
        app->screen_frame++;
    }
    end();

    for(uint8_t a = 1; a < 8; a += 2) {
        char name[48];
        s = pet(DigiviceFormWyvolt);
        s.area = a;
        s.distance = (uint16_t)(digivice_rules_area_length(a) * 6 / 10);
        s.mid_state = 2;
        new_app(&s);
        digivice_app_set_screen(app, DigiviceScreenExpedition);
        app->trail_scroll = 70 + a * 9;
        snprintf(name, sizeof(name), "shot-trail-area%u", (unsigned)(a + 1));
        begin(name);
        snap();
        end();
    }

    // Dusk on a trail.
    uint32_t keep = g_now;
    g_now = DAY(21) + 1800;
    s = pet(DigiviceFormWyvolt);
    s.area = 2;
    s.distance = 400;
    new_app(&s);
    digivice_app_set_screen(app, DigiviceScreenExpedition);
    app->trail_scroll = 22;
    begin("shot-trail-night");
    snap();
    end();
    s.area = 1;
    s.city = 1;
    new_app(&s);
    digivice_app_set_screen(app, DigiviceScreenExpedition);
    app->trail_scroll = 30;
    app->screen_frame = 99;
    begin("shot-trail-city-barcelona-night");
    snap();
    end();
    g_now = keep;

    // Cities replace the area scenery with a skyline.
    for(uint8_t c = 1; c <= 5; c++) {
        char name[48];
        static const char* const slug[] = {"", "barcelona", "madrid", "valencia", "sevilla", "bilbao"};
        s = pet(DigiviceFormFuzzcan);
        s.area = 1;
        s.city = c;
        s.distance = 500;
        new_app(&s);
        digivice_app_set_screen(app, DigiviceScreenExpedition);
        app->trail_scroll = 30;
        app->screen_frame = c == 1 ? 4 : 99;
        snprintf(name, sizeof(name), "shot-trail-city-%s", slug[c]);
        begin(name);
        snap();
        end();
    }

    s = pet(DigiviceFormFuzzcan);
    s.cleared = 0x07;
    s.area = 3;
    s.mid_state = 1;
    new_app(&s);
    digivice_app_set_screen(app, DigiviceScreenMap);
    app->map_sel = 4;
    begin("shot-map");
    snap();
    end();

    s = pet(DigiviceFormFuzzcan);
    s.area = 2;
    new_app(&s);
    digivice_app_set_screen(app, DigiviceScreenGate);
    begin("shot-boss-gate");
    snap();
    end();
}

static void scene_battle(void) {
    DigiviceState s = pet(DigiviceFormFuzzcan);
    s.area = 1;
    s.strength = 18;
    s.item = DigiviceItemShield;
    new_app(&s);
    fight(DigiviceEncounterTrail, 0);
    const uint32_t lead[] = {3, 11, 19, 40};
    begin("anim-battle-intro");
    intro(lead, 4);
    end();
    begin("shot-battle-intro");
    app->screen_frame = 40;
    snap();
    end();

    to_menu();
    app->menu_sel = 0;
    begin("shot-battle-menu");
    snap();
    end();

    battle_set_phase(app, DigiviceBattlePhaseMoves);
    app->move_sel = 1;
    begin("shot-battle-moves");
    snap();
    end();

    battle_set_phase(app, DigiviceBattlePhaseTiming);
    app->zone_start = 9;
    app->cursor = 10;
    begin("shot-battle-timing");
    snap();
    end();

    // A full player step, played by the real tick: ring, shot, impact, messages.
    new_app(&s);
    fight(DigiviceEncounterTrail, 0);
    to_menu();
    app->menu_sel = 0;
    battle_commit(app, 1, false);
    begin("anim-battle-attack");
    for(int i = 0; i < 70 && app->phase == DigiviceBattlePhaseTurn; i++) {
        snap();
        tick();
    }
    end();

    // Victory screen.
    new_app(&s);
    fight(DigiviceEncounterTrail, 0);
    to_menu();
    app->result = DigiviceBattleResultWin;
    app->reward = (DigiviceReward){.exp = 24, .item = DigiviceItemShield};
    battle_set_phase(app, DigiviceBattlePhaseEnd);
    app->screen_frame = 16;
    begin("shot-battle-victory");
    snap();
    end();

    // Defeat.
    app->result = DigiviceBattleResultLose;
    app->reward = (DigiviceReward){0};
    app->screen_frame = 12;
    begin("shot-battle-defeat");
    snap();
    end();
}

static void scene_boss(void) {
    DigiviceState s = pet(DigiviceFormWyvolt);
    s.area = 2;
    s.strength = 22;
    new_app(&s);
    fight(DigiviceEncounterBoss, DIGIVICE_ENC_NONE);
    const uint32_t lead[] = {4, 11, 20, 40};
    begin("anim-boss-intro");
    intro(lead, 4);
    end();
    app->screen_frame = 40;
    begin("shot-boss-intro");
    snap();
    end();

    to_menu();
    begin("shot-boss-battle");
    snap();
    end();

    battle_commit(app, 2, false);
    for(int i = 0; i < 17; i++) tick();
    begin("shot-boss-impact");
    snap();
    end();

    for(uint8_t a = 0; a < 8; a += 7) {
        char name[48];
        s = pet(DigiviceFormWyvolt);
        s.area = a;
        new_app(&s);
        fight(DigiviceEncounterBoss, DIGIVICE_ENC_NONE);
        to_menu();
        snprintf(name, sizeof(name), "shot-boss-area%u", (unsigned)(a + 1));
        begin(name);
        snap();
        end();
    }
}

static void scene_homage(void) {
    DigiviceState s = pet(DigiviceFormFuzzcan);
    s.area = 3;
    s.strength = 18;
    s.caught = 0x0005;
    s.companion = 0;
    new_app(&s);
    fight(DigiviceEncounterTrail, DIGIVICE_ENC_HOMAGE + 7);
    const uint32_t lead[] = {3, 5, 19, 40};
    begin("anim-homage-intro");
    intro(lead, 4);
    end();

    for(uint8_t id = 0; id < 9; id += 4) {
        char name[48];
        new_app(&s);
        fight(DigiviceEncounterTrail, (uint8_t)(DIGIVICE_ENC_HOMAGE + id));
        to_menu();
        snprintf(name, sizeof(name), "shot-homage-battle-%u", (unsigned)id);
        begin(name);
        snap();
        end();
    }

    // Capture: dizzy, ring, then caught.
    new_app(&s);
    fight(DigiviceEncounterTrail, DIGIVICE_ENC_HOMAGE + 4);
    to_menu();
    app->result = DigiviceBattleResultWin;
    app->reward = (DigiviceReward){.exp = 18, .capture = DigiviceCaptureCaught, .partner = 4};
    battle_set_phase(app, DigiviceBattlePhaseEnd);
    begin("anim-homage-capture");
    for(uint32_t f = 0; f < 56; f++) {
        app->screen_frame = f;
        app->frame++;
        snap();
    }
    end();
    app->screen_frame = 50;
    begin("shot-homage-captured");
    snap();
    end();

    // The nest, with a companion.
    s.caught = 0x01FF;
    s.companion = 7;
    new_app(&s);
    app->nido_sel = 7;
    digivice_app_set_screen(app, DigiviceScreenNido);
    begin("anim-nest");
    for(int i = 0; i < 32; i++) {
        snap();
        app->frame++;
    }
    end();
    s.caught = 0x006B;
    s.companion = 3;
    new_app(&s);
    app->nido_sel = 3;
    app->frame = 9;
    digivice_app_set_screen(app, DigiviceScreenNido);
    begin("shot-nest");
    snap();
    end();

    // Walking beside the pet.
    new_app(&s);
    app->frame = 16;
    begin("shot-companion-walk");
    snap();
    end();

    // City monster battle.
    s = pet(DigiviceFormFuzzcan);
    s.area = 1;
    s.city = 1;
    new_app(&s);
    fight(DigiviceEncounterTrail, DIGIVICE_ENC_CITY + 0);
    to_menu();
    begin("shot-city-monster-battle");
    snap();
    end();
    app->phase = DigiviceBattlePhaseMenu;
    app->screen = DigiviceScreenEncounter;
    app->screen_frame = 4;
    begin("shot-city-event-intro");
    snap();
    end();
}

static void scene_evolution(void) {
    DigiviceState s = pet(DigiviceFormFuzzcan);
    s.evo_from = DigiviceFormEstatik;
    new_app(&s);
    digivice_app_set_screen(app, DigiviceScreenEvolution);
    begin("anim-evolution");
    for(uint32_t f = 0; f <= DIGIVICE_EVO_FRAMES; f++) {
        app->screen_frame = f;
        app->frame = f;
        snap();
    }
    end();
    begin("sheet-evolution");
    const uint32_t beats[] = {8, 20, 32, 46};
    for(size_t i = 0; i < 4; i++) {
        app->screen_frame = beats[i];
        app->frame = beats[i];
        snap();
    }
    end();
}

static void scene_bestiary(void) {
    new_app(&(DigiviceState){0});
    S = pet(DigiviceFormFuzzcan);
    app->state = S;
    app->dex_sel = DigiviceFormWyvolt;
    digivice_app_set_screen(app, DigiviceScreenDex);
    begin("shot-bestiary-entry");
    snap();
    end();

    // The roster grids, ordered by stage. The app shows one form at a time, so these panels are
    // composed from the same draw calls: young forms five per row, evolved ones three per panel.
    uint8_t young[DigiviceFormCount];
    uint8_t grown[DigiviceFormCount];
    size_t ny = 0;
    size_t ng = 0;
    for(uint8_t stage = 0; stage < DigiviceStageCount; stage++) {
        for(uint8_t f = 0; f < DigiviceFormCount; f++) {
            const DigiviceRosterEntry* e = digivice_roster(f);
            // Forms 1..7 only exist for old saves and VIRUBYTE is the failure form.
            bool listed = e && e->stage == stage && (f == DigiviceFormByteegg || f >= DigiviceFormPipo);
            if(!listed) continue;
            if(stage <= DigiviceStageRookie) young[ny++] = f;
            else grown[ng++] = f;
        }
    }
    begin("sheet-bestiary-young");
    canvas_clear(cv);
    reset_canvas();
    for(size_t k = 0; k < ny && k < 10; k++) {
        const DigiviceRosterEntry* e = digivice_roster(young[k]);
        int32_t cx = 13 + (int32_t)(k % 5) * 25;
        int32_t cy = (k / 5) ? 49 : 17;
        digivice_draw_form(cv, (DigiviceForm)young[k], cx, cy, 3, DigivicePoseIdle, false);
        canvas_set_color(cv, ColorBlack);
        digivice_draw_text(cv, cx - (int32_t)digivice_draw_text_width(cv, e->name) / 2, cy + 12, e->name);
    }
    emit();
    end();

    begin("sheet-bestiary-evolved");
    for(size_t i = 0; i < ng; i += 3) {
        canvas_clear(cv);
        reset_canvas();
        for(size_t k = 0; k < 3 && i + k < ng; k++) {
            const DigiviceRosterEntry* e = digivice_roster(grown[i + k]);
            int32_t cx = 22 + (int32_t)k * 42;
            digivice_draw_form(cv, (DigiviceForm)grown[i + k], cx, 32, 3, DigivicePoseIdle, false);
            canvas_set_color(cv, ColorBlack);
            digivice_draw_text(cv, cx - (int32_t)digivice_draw_text_width(cv, e->name) / 2, 56, e->name);
        }
        emit();
    }
    end();
}

static void scene_monsters(void) {
    new_app(&(DigiviceState){0});
    S = pet(DigiviceFormFuzzcan);
    app->state = S;

    begin("sheet-trail-foes");
    for(int p = 0; p < 2; p++) {
        canvas_clear(cv);
        reset_canvas();
        for(int k = 0; k < 4; k++) {
            DigiviceFoe foe = (DigiviceFoe)(p * 4 + k);
            int32_t cx = 16 + k * 32;
            digivice_draw_foe(cv, foe, cx, 40, 3, DigivicePoseIdle, k % 2, false);
            canvas_set_color(cv, ColorBlack);
            const char* nm = digivice_expedition_foe_name(foe);
            digivice_draw_text(cv, cx - (int32_t)digivice_draw_text_width(cv, nm) / 2, 50, nm);
        }
        emit();
    }
    end();

    begin("sheet-city-monsters");
    for(int p = 0; p < 2; p++) {
        canvas_clear(cv);
        reset_canvas();
        for(int k = 0; k < 3 && p * 3 + k < 5; k++) {
            int idx = p * 3 + k;
            int32_t cx = 22 + k * 42;
            digivice_draw_city_monster(cv, (uint8_t)idx, cx, 40, 3, DigivicePoseIdle, false, false);
            canvas_set_color(cv, ColorBlack);
            const char* nm = digivice_city_monster_name((uint8_t)idx);
            digivice_draw_text(cv, cx - (int32_t)digivice_draw_text_width(cv, nm) / 2, 50, nm);
        }
        emit();
    }
    end();

    begin("sheet-bosses");
    for(int p = 0; p < 4; p++) {
        canvas_clear(cv);
        reset_canvas();
        for(int k = 0; k < 2; k++) {
            uint8_t a = (uint8_t)(p * 2 + k);
            int32_t cx = 32 + k * 64;
            digivice_draw_boss(cv, a, DigiviceFormFuzzcan, cx, 52, 3, false);
            canvas_set_color(cv, ColorBlack);
            const char* nm = digivice_expedition_boss_name(a);
            digivice_draw_text(cv, cx - (int32_t)digivice_draw_text_width(cv, nm) / 2, 57, nm);
        }
        emit();
    }
    end();

    begin("sheet-homage-partners");
    for(int p = 0; p < 3; p++) {
        canvas_clear(cv);
        reset_canvas();
        for(int k = 0; k < 3; k++) {
            uint8_t id = (uint8_t)(p * 3 + k);
            int32_t cx = 22 + k * 42;
            digivice_draw_homage(cv, id, cx, 44, 3, DigivicePoseIdle, false, false, false);
            canvas_set_color(cv, ColorBlack);
            const char* nm = digivice_homage_name(id);
            digivice_draw_text(cv, cx - (int32_t)digivice_draw_text_width(cv, nm) / 2, 52, nm);
        }
        emit();
    }
    end();
}

int main(int argc, char** argv) {
    out_dir = argc > 1 ? argv[1] : ".";
    cv = canvas_alloc();
    scene_home();
    scene_menu();
    scene_feed();
    scene_train_profile();
    scene_trail();
    scene_battle();
    scene_boss();
    scene_homage();
    scene_evolution();
    scene_bestiary();
    scene_monsters();
    return 0;
}
