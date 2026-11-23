/* SPDX-License-Identifier: GPL-2.0-only */
/* Integration tests for the real WL_MENU selection consumer.
   WL_MENU.c is compiled unchanged; only its platform/UI neighbours are
   stubbed so navigation, bounds and rendering decisions can be observed
   without an SDL window or game assets. */
#include "engine/WL_MENU.h"
#include "engine/WL_DEF.h"
#include "engine/ID_CA.h"
#include "engine/ID_VH.h"
#include "engine/ID_US_1.h"
#include "engine/ID_IN.h"
#include "engine/ID_SD.h"
#include "engine/WL_SAVE.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #x); exit(1); } } while (0)

/* ---- Minimal UI/platform stubs ---------------------------------------- */
int fontnumber;
w3r_byte fontcolor;
int PrintX, PrintY, WindowX;
bool Keyboard[128];

static bool pressed[128];
static char printed[64][160];
static unsigned printed_count;
static int pic_x[64], pic_y[64], pic_chunk[64];
static unsigned pic_count;

bool CA_Fail(const char *format, ...) { (void)format; return false; }
const char *Save_Directory(void) { return ""; }
bool GameCanSave(void) { return false; }
bool GameRunning(void) { return false; }
void IN_ClearKeysDown(void) { memset(pressed, 0, sizeof(pressed)); }
bool IN_ConsumePress(unsigned scan)
{
    if (scan < 128 && pressed[scan]) { pressed[scan] = false; return true; }
    return false;
}
char IN_NextASCII(void) { return 0; }
bool SD_DigitizedMode(void) { return false; }
bool SD_PlaySound(soundnames sound, unsigned left, unsigned right)
{ (void)sound; (void)left; (void)right; return true; }
void SD_SetVolumes(unsigned music, unsigned effects) { (void)music; (void)effects; }
void SD_SetEnabled(bool music, bool effects) { (void)music; (void)effects; }
void SD_GetSettings(unsigned *music, unsigned *effects, bool *music_on, bool *effects_on)
{ if (music) *music = 0; if (effects) *effects = 0; if (music_on) *music_on = false; if (effects_on) *effects_on = false; }
bool VW_ValidateFont(int number) { (void)number; return true; }
bool VW_MeasurePropString(const char *text, int *width, int *height)
{ (void)text; if (width) *width = 0; if (height) *height = 0; return true; }
bool VWB_DrawPic(int x, int y, int chunk)
{
    if (pic_count < 64) { pic_x[pic_count] = x; pic_y[pic_count] = y; pic_chunk[pic_count] = chunk; }
    ++pic_count;
    return true;
}
void VWB_Bar(int x, int y, int width, int height, w3r_byte color)
{ (void)x; (void)y; (void)width; (void)height; (void)color; }
void VWB_Hlin(int x1, int x2, int y, w3r_byte color) { (void)x1; (void)x2; (void)y; (void)color; }
void VWB_Vlin(int y1, int y2, int x, w3r_byte color) { (void)y1; (void)y2; (void)x; (void)color; }
bool US_Print(const char *text)
{
    if (printed_count < 64) {
        (void)snprintf(printed[printed_count], sizeof(printed[0]), "%s", text ? text : "");
    }
    ++printed_count;
    return true;
}

/* ---- Observation helpers ---------------------------------------------- */
static void ResetCapture(void)
{
    memset(pressed, 0, sizeof(pressed));
    memset(Keyboard, 0, sizeof(Keyboard));
    printed_count = 0;
    pic_count = 0;
}
static void Press(unsigned scan) { CHECK(scan < 128); pressed[scan] = true; }
static bool PrintedContains(const char *needle)
{
    for (unsigned i = 0; i < printed_count && i < 64; ++i)
        if (strstr(printed[i], needle)) return true;
    return false;
}
static unsigned PrintedMatching(const char *needle)
{
    unsigned n = 0;
    for (unsigned i = 0; i < printed_count && i < 64; ++i)
        if (strstr(printed[i], needle)) ++n;
    return n;
}
static bool PictureChunk(int chunk)
{
    for (unsigned i = 0; i < pic_count && i < 64; ++i)
        if (pic_chunk[i] == chunk) return true;
    return false;
}
static bool PictureAt(int x, int y, int chunk)
{
    for (unsigned i = 0; i < pic_count && i < 64; ++i)
        if (pic_x[i] == x && pic_y[i] == y && pic_chunk[i] == chunk) return true;
    return false;
}

/* ---- Definitions ------------------------------------------------------ */
static const char *alt_eps[] = { "Alt One", "Alt Two", "Alt Three" };
static const char *alt_diff[] = { "Gentle", "Brutal" };
static const char *alt_juke[] = { "A", "B", "C", "D" };
static const game_definition_t alt = {
    .episode_names = alt_eps, .episode_count = 3,
    .difficulty_names = alt_diff, .difficulty_count = 2,
    .jukebox_names = alt_juke, .jukebox_count = 4,
};

static const char *wide_eps[] = { "E1", "E2", "E3", "E4", "E5", "E6", "E7", "E8" };
static const char *wide_diff[] = { "D1", "D2", "D3", "D4", "D5", "D6" };
static const game_definition_t wide = {
    .episode_names = wide_eps, .episode_count = 8,
    .difficulty_names = wide_diff, .difficulty_count = 6,
    .jukebox_names = alt_juke, .jukebox_count = 4,
};

static const game_definition_t zero = {
    .episode_names = NULL, .episode_count = 0,
    .difficulty_names = NULL, .difficulty_count = 0,
    .jukebox_names = NULL, .jukebox_count = 0,
};

static const game_definition_t invalid = {
    .episode_names = NULL, .episode_count = 1,
    .difficulty_names = NULL, .difficulty_count = 0,
    .jukebox_names = NULL, .jukebox_count = 0,
};

/* ---- Tests ------------------------------------------------------------ */
static void TestDefaultEpisodeRendering(void)
{
    const game_definition_t *def = GameDefinition_Default();
    ResetCapture();
    CHECK(DrawNewSelection(def, true, 0));
    CHECK(PrintedContains("Which episode to play?"));
    CHECK(PrintedContains("Episode 1\nEscape from Wolfenstein"));
    CHECK(PrintedContains("Episode 6\nConfrontation"));
    CHECK(PrintedMatching("Episode ") == 6);
    for (int i = 0; i < 6; ++i) CHECK(PictureChunk(C_EPISODE1PIC + i));
    CHECK(!PictureChunk(C_EPISODE1PIC + 6));
    CHECK(PictureAt(10, 24, C_CURSOR1PIC));
}

static void TestDefaultDifficultyRendering(void)
{
    const game_definition_t *def = GameDefinition_Default();
    ResetCapture();
    CHECK(DrawNewSelection(def, false, 3));
    CHECK(PrintedContains("How tough are you?"));
    CHECK(PrintedContains("Can I play, Daddy?"));
    CHECK(PrintedContains("I am Death incarnate!"));
    CHECK(PictureAt(235, 107, C_BABYMODEPIC + 3));
    CHECK(!PictureChunk(C_BABYMODEPIC + 4));
    CHECK(PictureAt(50, 98 + 3 * 13, C_CURSOR1PIC));
}

static void TestAlternativeRendering(void)
{
    ResetCapture();
    CHECK(DrawNewSelection(&alt, true, 2));
    CHECK(PrintedMatching("Episode ") == 3);
    CHECK(PrintedContains("Alt One"));
    CHECK(PrintedContains("Alt Three"));
    CHECK(!PrintedContains("Confrontation"));
    for (int i = 0; i < 3; ++i) CHECK(PictureChunk(C_EPISODE1PIC + i));
    CHECK(!PictureChunk(C_EPISODE1PIC + 3));
    CHECK(PictureAt(10, 24 + 2 * 26, C_CURSOR1PIC));

    ResetCapture();
    CHECK(DrawNewSelection(&alt, false, 1));
    CHECK(PrintedContains("Gentle"));
    CHECK(PrintedContains("Brutal"));
    CHECK(!PrintedContains("I am Death incarnate!"));
    CHECK(PictureAt(235, 107, C_BABYMODEPIC + 1));
    CHECK(!PictureChunk(C_BABYMODEPIC + 2));
    CHECK(PictureAt(50, 98 + 1 * 13, C_CURSOR1PIC));

    /* Out-of-range rendering falls back to the first slot, not an
       out-of-bounds episode picture or cursor position. */
    ResetCapture();
    CHECK(DrawNewSelection(&alt, true, 99));
    CHECK(PictureAt(10, 24, C_CURSOR1PIC));
    CHECK(!PictureChunk(C_EPISODE1PIC + 3));
}

static void TestLegacyDisplayLimits(void)
{
    ResetCapture();
    CHECK(DrawNewSelection(&wide, true, 0));
    CHECK(PrintedMatching("Episode ") == 6);
    CHECK(PrintedContains("E6"));
    CHECK(!PrintedContains("E7"));
    for (int i = 0; i < 6; ++i) CHECK(PictureChunk(C_EPISODE1PIC + i));
    CHECK(!PictureChunk(C_EPISODE1PIC + 6));

    ResetCapture();
    CHECK(DrawNewSelection(&wide, false, 0));
    CHECK(PrintedContains("D4"));
    CHECK(!PrintedContains("D5"));
    CHECK(PictureAt(235, 107, C_BABYMODEPIC + 0));
    CHECK(!PictureChunk(C_BABYMODEPIC + 4));
}

static void TestNavigationCounts(void)
{
    unsigned choice;
    int action;

    ResetCapture();
    choice = 0;
    Press(sc_DownArrow);
    action = HandleNewSelection(&alt, 1000, true, &choice);
    CHECK(action == 0 && choice == 1);
    Press(sc_DownArrow);
    CHECK(HandleNewSelection(&alt, 1020, true, &choice) == 0 && choice == 2);
    Press(sc_DownArrow);
    CHECK(HandleNewSelection(&alt, 1040, true, &choice) == 0 && choice == 0);
    Press(sc_UpArrow);
    CHECK(HandleNewSelection(&alt, 1060, true, &choice) == 0 && choice == 2);

    choice = 0;
    Press(sc_DownArrow);
    CHECK(HandleNewSelection(&alt, 1080, false, &choice) == 0 && choice == 1);
    Press(sc_DownArrow);
    CHECK(HandleNewSelection(&alt, 1100, false, &choice) == 0 && choice == 0);
    Press(sc_UpArrow);
    CHECK(HandleNewSelection(&alt, 1120, false, &choice) == 0 && choice == 1);

    /* Oversized definitions never navigate beyond the legacy slots. */
    choice = 5;
    Press(sc_DownArrow);
    CHECK(HandleNewSelection(&wide, 1140, true, &choice) == 0 && choice == 0);
    choice = 0;
    Press(sc_UpArrow);
    CHECK(HandleNewSelection(&wide, 1160, true, &choice) == 0 && choice == 5);

    choice = 3;
    Press(sc_DownArrow);
    CHECK(HandleNewSelection(&wide, 1180, false, &choice) == 0 && choice == 0);
    choice = 0;
    Press(sc_UpArrow);
    CHECK(HandleNewSelection(&wide, 1200, false, &choice) == 0 && choice == 3);
}

static void TestInvalidAndNullChoices(void)
{
    unsigned choice = 99;

    ResetCapture();
    /* Out-of-range is normalized before input is consumed. */
    CHECK(HandleNewSelection(&alt, 2000, true, &choice) == 0);
    CHECK(choice == 0);

    choice = 99;
    Press(sc_Enter);
    CHECK(HandleNewSelection(&alt, 2020, true, &choice) == 1);
    CHECK(choice == 0);

    choice = 99;
    Press(sc_Space);
    CHECK(HandleNewSelection(&alt, 2040, false, &choice) == 1);
    CHECK(choice == 0);

    /* A valid current selection is preserved on acceptance. */
    choice = 2;
    Press(sc_Enter);
    CHECK(HandleNewSelection(&alt, 2060, true, &choice) == 1 && choice == 2);

    choice = 1;
    Press(sc_Escape);
    CHECK(HandleNewSelection(&alt, 2080, false, &choice) == -1 && choice == 1);

    CHECK(HandleNewSelection(&alt, 2100, true, NULL) == -3);
    CHECK(HandleNewSelection(&invalid, 2120, true, &choice) == -3);
    CHECK(!DrawNewSelection(&invalid, true, 0));
}

static void TestZeroCountDefinition(void)
{
    unsigned choice = 0;

    ResetCapture();
    CHECK(GameDefinition_Valid(&zero));
    ResetCapture();
    CHECK(!DrawNewSelection(&zero, true, 0));
    CHECK(pic_count == 0);
    CHECK(!DrawNewSelection(&zero, false, 0));
    CHECK(pic_count == 0);

    Press(sc_Enter);
    CHECK(HandleNewSelection(&zero, 3000, true, &choice) == -3);
    Press(sc_Space);
    CHECK(HandleNewSelection(&zero, 3020, false, &choice) == -3);
}

static void TestLegacyDefaultNavigation(void)
{
    const game_definition_t *def = GameDefinition_Default();
    unsigned choice = 5;
    ResetCapture();
    Press(sc_DownArrow);
    CHECK(HandleNewSelection(def, 4000, true, &choice) == 0 && choice == 0);
    choice = 0;
    Press(sc_UpArrow);
    CHECK(HandleNewSelection(def, 4020, true, &choice) == 0 && choice == 5);

    choice = 3;
    Press(sc_DownArrow);
    CHECK(HandleNewSelection(def, 4040, false, &choice) == 0 && choice == 0);
    choice = 0;
    Press(sc_UpArrow);
    CHECK(HandleNewSelection(def, 4060, false, &choice) == 0 && choice == 3);
}

int main(void)
{
    TestDefaultEpisodeRendering();
    TestDefaultDifficultyRendering();
    TestAlternativeRendering();
    TestLegacyDisplayLimits();
    TestNavigationCounts();
    TestInvalidAndNullChoices();
    TestZeroCountDefinition();
    TestLegacyDefaultNavigation();
    puts("menu_selection_tests: PASS");
    return 0;
}
