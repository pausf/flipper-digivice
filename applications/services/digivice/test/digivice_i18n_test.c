// Host-side test for the digivice strings. Run: applications/services/digivice/test/run.sh
// Arguments: source files that must stay ASCII-only.
#include "../digivice_i18n.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int failures = 0;

#define CHECK(cond)                                                \
    do {                                                           \
        if(!(cond)) {                                              \
            printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
            failures++;                                            \
        }                                                          \
    } while(0)

static void test_width(void) {
    CHECK(digivice_i18n_text_width("") == 0);
    CHECK(digivice_i18n_text_width("A") == 3);
    CHECK(digivice_i18n_text_width("COMER") == 19);
    // Inverted exclamation + N tilde + "AM!": 5 glyphs in 7 bytes.
    CHECK(digivice_i18n_text_width("\xC2\xA1\xC3\x91" "AM!") == 19);
}

static void test_every_string_present_and_fits(void) {
    for(int lang = 0; lang < DigiviceLangCount; lang++) {
        for(int id = 0; id < DigiviceStrCount; id++) {
            const char* text = digivice_str((DigiviceLang)lang, (DigiviceStr)id);
            uint16_t width = digivice_i18n_text_width(text);
            uint16_t budget = digivice_i18n_budget((DigiviceStr)id);
            if(text[0] == '\0') printf("missing: lang %d id %d\n", lang, id);
            CHECK(text[0] != '\0');
            if(width > budget) {
                printf(
                    "too wide: lang %d id %d is %u px, budget %u\n",
                    lang,
                    id,
                    (unsigned)width,
                    (unsigned)budget);
            }
            CHECK(width <= budget);
            CHECK(budget > 0);
        }
    }
}

static void test_out_of_range_is_safe(void) {
    CHECK(digivice_str(DigiviceLangEs, DigiviceStrCount)[0] == '\0');
    CHECK(strcmp(digivice_str((DigiviceLang)7, DigiviceStrMenuFeed), "COMER") == 0);
    CHECK(strcmp(digivice_str(DigiviceLangEn, DigiviceStrMenuFeed), "FEED") == 0);
}

static void test_utf8_is_well_formed(void) {
    for(int lang = 0; lang < DigiviceLangCount; lang++) {
        for(int id = 0; id < DigiviceStrCount; id++) {
            const unsigned char* p =
                (const unsigned char*)digivice_str((DigiviceLang)lang, (DigiviceStr)id);
            for(; *p; p++) {
                if(*p < 0x80) continue;
                // Only 2-byte sequences for U+0080..U+00FF, the Latin-1 range the font holds.
                bool ok = (p[0] == 0xC2 || p[0] == 0xC3) && (p[1] & 0xC0) == 0x80;
                CHECK(ok);
                if(!ok) break;
                p++;
            }
        }
    }
}

static void test_sources_are_ascii(int argc, char** argv) {
    for(int i = 1; i < argc; i++) {
        FILE* file = fopen(argv[i], "rb");
        CHECK(file != NULL);
        if(!file) continue;
        int c;
        long line = 1;
        while((c = fgetc(file)) != EOF) {
            if(c == '\n') line++;
            bool ascii = c == '\n' || c == '\t' || c == '\r' || (c >= 0x20 && c < 0x7F);
            if(!ascii) {
                printf("non-ASCII byte 0x%02X in %s:%ld\n", (unsigned)c, argv[i], line);
                failures++;
                break;
            }
        }
        fclose(file);
    }
}

int main(int argc, char** argv) {
    test_width();
    test_every_string_present_and_fits();
    test_out_of_range_is_safe();
    test_utf8_is_well_formed();
    test_sources_are_ascii(argc, argv);

    if(failures) {
        printf("%d check(s) failed\n", failures);
        return EXIT_FAILURE;
    }
    printf("digivice_i18n: all checks passed\n");
    return EXIT_SUCCESS;
}
