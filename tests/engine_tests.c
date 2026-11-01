/* SPDX-License-Identifier: GPL-2.0-only */
#include "engine/ID_CA.h"
#include "engine/ID_VL.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "%s:%d: %s (%s)\n", __FILE__, __LINE__, #x, CA_Error()); exit(1); } } while (0)

static void TestMemory(void)
{
    memptr a = NULL, b = NULL, foreign = NULL;
    MM_Startup();
    CHECK(!MM_GetPtr(NULL, 4));
    CHECK(!MM_GetPtr(&a, 0));
    CHECK(!MM_GetPtr(&a, SIZE_MAX));
    CHECK(!MM_GetPtr(&a, MM_MAX_BYTES + 1));
    CHECK(!a && !MM_TotalBytes());
    CHECK(MM_GetPtr(&a, 8));
    CHECK(MM_GetPtr(&b, 16));
    for (size_t i = 0; i < 8; ++i) CHECK(((w3r_byte *)a)[i] == 0);
    CHECK(!MM_GetPtr(&a, 4));
    CHECK(MM_TotalBytes() == 24);
    CHECK(!MM_FreePtr(&foreign));
    CHECK(!MM_SetPurge(&a, 4));
    CHECK(MM_SetPurge(&a, 3));
    CHECK(MM_SetLock(&a, true));
    MM_SortMem();
    CHECK(a && b && MM_TotalBytes() == 24);
    CHECK(MM_SetLock(&a, false));
    MM_SortMem();
    CHECK(!a && b && MM_TotalBytes() == 16);
    CHECK(MM_FreePtr(&b));
    CHECK(!MM_FreePtr(&b));
    CHECK(!MM_TotalBytes());
    CHECK(MM_GetPtr(&a, 1));
    MM_Shutdown();
    CHECK(!a && !MM_TotalBytes());
    MM_Shutdown();
}

static void TestHuffman(void)
{
    huffnode table[255] = {{0, 0}};
    w3r_byte source = 0xa6, out[10];
    memset(out, 0xcc, sizeof(out));
    table[254].bit0 = 'A'; table[254].bit1 = 'B';
    CHECK(CAL_ValidateHuffman(table));
    CHECK(CAL_HuffExpand(&source, 1, out, 8, table));
    CHECK(memcmp(out, "ABBAABAB", 8) == 0 && out[8] == 0xcc);
    CHECK(!CAL_HuffExpand(&source, 1, out, 9, table));
    CHECK(out[9] == 0xcc);
    CHECK(!CAL_HuffExpand(&source, 0, out, 1, table));
    CHECK(!CAL_HuffExpand(NULL, 1, out, 1, table));
    CHECK(!CAL_HuffExpand(&source, 1, NULL, 1, table));
    CHECK(CAL_HuffExpand(NULL, 0, NULL, 0, table));
    table[254].bit0 = 256; table[0].bit0 = 'C'; table[0].bit1 = 'D';
    source = 0x18;
    CHECK(CAL_HuffExpand(&source, 1, out, 4, table));
    CHECK(memcmp(out, "CDBC", 4) == 0);
    table[254].bit0 = 511;
    CHECK(!CAL_ValidateHuffman(table));
    CHECK(!CAL_HuffExpand(&source, 1, out, 1, table));
    table[254].bit0 = 510; source = 0; /* Cycle back to the root. */
    CHECK(!CAL_ValidateHuffman(table));
    CHECK(!CAL_HuffExpand(&source, 1, out, 1, table));
    /* Deterministic malformed-tree/stream corpus, bounded output with canaries. */
    uint32_t random = 0x12345678;
    for (unsigned iteration = 0; iteration < 2000; ++iteration) {
        for (size_t i = 0; i < 255; ++i) {
            random = random * UINT32_C(1664525) + UINT32_C(1013904223);
            table[i].bit0 = (uint16_t)(random & 1023);
            table[i].bit1 = (uint16_t)((random >> 10) & 1023);
        }
        w3r_byte guarded[34], bits[8];
        memset(guarded, 0xc7, sizeof(guarded));
        for (size_t i = 0; i < sizeof(bits); ++i) bits[i] = (w3r_byte)(random >> (i % 4));
        (void)CAL_HuffExpand(bits, sizeof(bits), guarded + 1, 32, table);
        CHECK(guarded[0] == 0xc7 && guarded[33] == 0xc7);
    }
}

static void TestVideoBuffers(void)
{
    const w3r_byte planes[16] = {0,4,8,12, 1,5,9,13, 2,6,10,14, 3,7,11,15};
    static w3r_byte rgba[VL_PIXELS * 4];
    w3r_byte invalid[768];
    memset(framebuffer, 0xee, VL_PIXELS);
    CHECK(VL_MemToScreen(planes, sizeof(planes), 8, 2, 3, 4));
    for (size_t row = 0; row < 2; ++row)
        for (size_t x = 0; x < 8; ++x)
            CHECK(framebuffer[(row + 4) * 320 + x + 3] == row * 8 + x);
    CHECK(framebuffer[4 * 320 + 2] == 0xee && framebuffer[4 * 320 + 11] == 0xee);
    CHECK(!VL_MemToScreen(planes, 15, 8, 2, 0, 0));
    CHECK(!VL_MemToScreen(planes, 16, 7, 2, 0, 0));
    CHECK(!VL_MemToScreen(planes, 16, 8, 2, -1, 0));
    CHECK(!VL_MemToScreen(planes, 16, 8, 2, 313, 0));
    CHECK(!VL_MemToScreen(planes, 16, 8, 2, 0, 199));
    CHECK(!VL_MemToScreen(NULL, 16, 8, 2, 0, 0));
    CHECK(framebuffer[0] == 0xee); /* Failures must not partially write. */
    CHECK(VL_SetPalette(gamepal, sizeof(gamepal)));
    memcpy(invalid, gamepal, sizeof(invalid)); invalid[7] = 64;
    CHECK(!VL_SetPalette(invalid, sizeof(invalid)));
    CHECK(!VL_SetPalette(gamepal, 767));
    CHECK(memcmp(palette, gamepal, sizeof(palette)) == 0);
    for (size_t i = 0; i < VL_PIXELS; ++i) framebuffer[i] = (w3r_byte)(i % 256);
    CHECK(VL_ConvertRGBA(rgba, sizeof(rgba)));
    CHECK(!VL_ConvertRGBA(rgba, sizeof(rgba) - 1));
    for (size_t i = 0; i < VL_PIXELS; ++i) {
        for (size_t channel = 0; channel < 3; ++channel) {
            unsigned dac = gamepal[(i % 256) * 3 + channel];
            CHECK(rgba[i * 4 + channel] == (dac * 4 + dac / 16));
        }
        CHECK(rgba[i * 4 + 3] == 255);
    }
    CHECK(gamepal[0] == 0 && gamepal[3] == 0 && gamepal[5] == 42);
}

static void TestAssets(const char *directory, const char *dump)
{
    static w3r_byte rgba[VL_PIXELS * 4];
    MM_Startup();
    CHECK(CA_Startup(directory));
    CHECK(VL_SetPalette(gamepal, sizeof(gamepal)));
    CHECK(CA_CacheScreen(TITLEPIC));
    CHECK(grsizes[TITLEPIC] == VL_PIXELS);
    memptr cached = grsegs[TITLEPIC];
    size_t used = MM_TotalBytes();
    CHECK(CA_CacheGrChunk(TITLEPIC) && grsegs[TITLEPIC] == cached && MM_TotalBytes() == used);
    CHECK(!CA_CacheGrChunk(-1) && !CA_CacheGrChunk(NUMCHUNKS));
    CHECK(!CA_CacheScreen(STARTFONT));
    CHECK(MM_SetPurge(&grsegs[TITLEPIC], 3) && MM_SetLock(&grsegs[TITLEPIC], true));
    MM_SortMem();
    CHECK(grsegs[TITLEPIC] == cached);
    CHECK(MM_SetLock(&grsegs[TITLEPIC], false));
    MM_SortMem();
    CHECK(!grsegs[TITLEPIC]);
    CHECK(CA_CacheScreen(TITLEPIC));
    CHECK(MM_TotalBytes() == used);
    CHECK(VL_ConvertRGBA(rgba, sizeof(rgba)));
    FILE *file = fopen(dump, "wb");
    CHECK(file);
    CHECK(fwrite(framebuffer, 1, VL_PIXELS, file) == VL_PIXELS);
    CHECK(fwrite(rgba, 1, sizeof(rgba), file) == sizeof(rgba));
    CHECK(fclose(file) == 0);
    CA_Shutdown();
    CHECK(MM_TotalBytes() == 0);
    CHECK(CA_Startup(directory)); /* Repeated lifecycle is clean. */
    CA_Shutdown(); MM_Shutdown();
    CHECK(!MM_TotalBytes());
}

int main(int argc, char **argv)
{
    TestMemory(); TestHuffman(); TestVideoBuffers();
    if (argc == 3) TestAssets(argv[1], argv[2]);
    puts("Engine memory, Huffman, framebuffer/palette and cache tests passed.");
    return 0;
}
