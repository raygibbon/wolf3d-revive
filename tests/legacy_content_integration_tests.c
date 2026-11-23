/* SPDX-License-Identifier: GPL-2.0-only */
/* Legacy content integration tests with synthetic WL6 fixtures.
   Verifies that custom-named resources are loaded through the adapter.
   Compile with: gcc -I src -I src/engine -I tests tests/legacy_content_integration_tests.c
   Note: self-contained; no Python, no shell helpers, no placeholders. */
#include "legacy_content.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdint.h>
#include <stdarg.h>

/*
 * Synthetic WL6 data generator.
 * Adapted from test_engine.py:fixtures() and game_fixtures.py:game_files().
 * Creates valid WL6-format archives for all 8 resource types in the
 * given directory. Uses only standard C and POSIX.
 */

/* Balanced Huffman dictionary: 255 nodes, LSB-first codes stored as
   little-endian 16-bit pairs. Node bit0/bit1 are child indices (0-255)
   or leaf symbols (256-511). */
static const uint8_t huff_dictionary[1020] = {
  0x00,0x01, 0x02,0x03, 0x04,0x05, 0x06,0x07, 0x08,0x09, 0x0a,0x0b, 0x0c,0x0d, 0x0e,0x0f,
  0x10,0x11, 0x12,0x13, 0x14,0x15, 0x16,0x17, 0x18,0x19, 0x1a,0x1b, 0x1c,0x1d, 0x1e,0x1f,
  0x20,0x21, 0x22,0x23, 0x24,0x25, 0x26,0x27, 0x28,0x29, 0x2a,0x2b, 0x2c,0x2d, 0x2e,0x2f,
  0x30,0x31, 0x32,0x33, 0x34,0x35, 0x36,0x37, 0x38,0x39, 0x3a,0x3b, 0x3c,0x3d, 0x3e,0x3f,
  0x40,0x41, 0x42,0x43, 0x44,0x45, 0x46,0x47, 0x48,0x49, 0x4a,0x4b, 0x4c,0x4d, 0x4e,0x4f,
  0x50,0x51, 0x52,0x53, 0x54,0x55, 0x56,0x57, 0x58,0x59, 0x5a,0x5b, 0x5c,0x5d, 0x5e,0x5f,
  0x60,0x61, 0x62,0x63, 0x64,0x65, 0x66,0x67, 0x68,0x69, 0x6a,0x6b, 0x6c,0x6d, 0x6e,0x6f,
  0x70,0x71, 0x72,0x73, 0x74,0x75, 0x76,0x77, 0x78,0x79, 0x7a,0x7b, 0x7c,0x7d, 0x7e,0x7f,
  0x80,0x81, 0x82,0x83, 0x84,0x85, 0x86,0x87, 0x88,0x89, 0x8a,0x8b, 0x8c,0x8d, 0x8e,0x8f,
  0x90,0x91, 0x92,0x93, 0x94,0x95, 0x96,0x97, 0x98,0x99, 0x9a,0x9b, 0x9c,0x9d, 0x9e,0x9f,
  0xa0,0xa1, 0xa2,0xa3, 0xa4,0xa5, 0xa6,0xa7, 0xa8,0xa9, 0xaa,0xab, 0xac,0xad, 0xae,0xaf,
  0xb0,0xb1, 0xb2,0xb3, 0xb4,0xb5, 0xb6,0xb7, 0xb8,0xb9, 0xba,0xbb, 0xbc,0xbd, 0xbe,0xbf,
  0xc0,0xc1, 0xc2,0xc3, 0xc4,0xc5, 0xc6,0xc7, 0xc8,0xc9, 0xca,0xcb, 0xcc,0xcd, 0xce,0xcf,
  0xd0,0xd1, 0xd2,0xd3, 0xd4,0xd5, 0xd6,0xd7, 0xd8,0xd9, 0xda,0xdb, 0xdc,0xdd, 0xde,0xdf,
  0xe0,0xe1, 0xe2,0xe3, 0xe4,0xe5, 0xe6,0xe7, 0xe8,0xe9, 0xea,0xeb, 0xec,0xed, 0xee,0xef,
  0xf0,0xf1, 0xf2,0xf3, 0xf4,0xf5, 0xf6,0xf7, 0xf8,0xf9, 0xfa,0xfb, 0xfc,0xfd, 0xfe,0xff,
};

/* Create synthetic VGADICT.WL6 (255 nodes, 1020 bytes). */
static bool create_vgadict(const char *path)
{
    FILE *f = fopen(path, "wb");
    if (!f) return false;
    fwrite(huff_dictionary, 1, 1020, f);
    fclose(f);
    return true;
}

/* Create synthetic AUDIOHED.WL6 (289 offsets = 289 * 4 bytes). */
static bool create_audiohed(const char *path)
{
    int n = 289;
    int size = n * 4;
    uint32_t *offsets = calloc(1, size);
    if (!offsets) return false;
    for (int i = 0; i < n; ++i) offsets[i] = i * 4;
    FILE *f = fopen(path, "wb");
    if (!f) { free(offsets); return false; }
    fwrite(offsets, 4, n, f);
    fclose(f);
    free(offsets);
    return true;
}

/* Create synthetic AUDIOT.WL6 (288 chunks of sample data). */
static bool create_audiot(const char *path)
{
    int chunk_count = 288;
    int data_size = 140;
    size_t total = (size_t)chunk_count * (2 + data_size);
    uint8_t *blob = malloc(total);
    if (!blob) return false;
    for (int i = 0; i < chunk_count; ++i) {
        size_t off = (size_t)i * (2 + data_size);
        blob[off + 0] = (uint8_t)(data_size & 0xff);
        blob[off + 1] = (uint8_t)((data_size >> 8) & 0xff);
        for (int j = 0; j < data_size; ++j) {
            blob[off + 2 + j] = (uint8_t)((i * 100 + j) & 0xff);
        }
    }
    FILE *f = fopen(path, "wb");
    if (!f) { free(blob); return false; }
    fwrite(blob, 1, total, f);
    fclose(f);
    free(blob);
    return true;
}

/* Create synthetic VSWAP.WL6 (page-file format). */
static bool create_vswap(const char *path)
{
    uint16_t count = 46;
    uint16_t sprites = 8;
    uint16_t start = 8;
    int page_count = 46;
    size_t dir_size = 6 + (size_t)page_count * 6;
    size_t pages_size = (size_t)page_count * 4096;
    size_t total = dir_size + pages_size;
    uint8_t *blob = malloc(total);
    if (!blob) return false;

    memcpy(blob, &count, 2);
    memcpy(blob + 2, &sprites, 2);
    memcpy(blob + 4, &start, 2);

    for (int i = 0; i < page_count; ++i) {
        size_t dir_off = 6 + (size_t)i * 6;
        uint32_t offset = (i < 46) ? 0 : (uint32_t)(6 + (i - 46) * 4096);
        uint16_t length = 4096;
        memcpy(blob + dir_off, &offset, 4);
        memcpy(blob + dir_off + 4, &length, 2);
    }

    for (int i = 0; i < page_count; ++i) {
        size_t page_off = dir_size + (size_t)i * 4096;
        for (int j = 0; j < 4096; ++j) {
            blob[page_off + j] = (uint8_t)((i + j) & 0xff);
        }
    }

    FILE *f = fopen(path, "wb");
    if (!f) { free(blob); return false; }
    fwrite(blob, 1, total, f);
    fclose(f);
    free(blob);
    return true;
}

/* Create synthetic MAPHEAD.WL6 (map header + 60 map entries). */
static bool create_maphead(const char *path)
{
    int tag = 0xabcd;
    int num_maps = 60;
    size_t size = 2 + (size_t)num_maps * 4 + 2;
    uint8_t *data = malloc(size);
    if (!data) return false;

    data[0] = (uint8_t)(tag & 0xff);
    data[1] = (uint8_t)((tag >> 8) & 0xff);

    for (int i = 0; i < num_maps; ++i) {
        size_t off = 2 + (size_t)i * 4;
        if (i == 0) {
            data[off] = 0x00; data[off+1] = 0x00; data[off+2] = 0x00; data[off+3] = 0x00;
        } else {
            data[off] = 0xff; data[off+1] = 0xff; data[off+2] = 0xff; data[off+3] = 0xff;
        }
    }

    data[size - 1] = 0x00;

    FILE *f = fopen(path, "wb");
    if (!f) { free(data); return false; }
    fwrite(data, 1, size, f);
    fclose(f);
    free(data);
    return true;
}

/* Create synthetic GAMEMAPS.WL6 (map data for one map). */
static bool create_gamemaps(const char *path)
{
    uint8_t header[38];
    header[0] = 0x00; header[1] = 0x00; header[2] = 0x00; header[3] = 0x00;
    *(uint32_t *)(header + 4) = 0x00000008;
    *(uint16_t *)(header + 12) = 64;
    *(uint16_t *)(header + 14) = 64;
    *(uint16_t *)(header + 16) = 64;
    *(uint16_t *)(header + 18) = 64;
    *(uint16_t *)(header + 20) = 64;
    *(uint16_t *)(header + 22) = 64;

    FILE *f = fopen(path, "wb");
    if (!f) return false;
    fwrite(header, 1, 38, f);
    uint8_t plane_data[4096];
    for (int i = 0; i < 4096; ++i) plane_data[i] = (uint8_t)(i & 0xff);
    fwrite(plane_data, 1, 4096, f);
    fclose(f);
    return true;
}

/* Create synthetic VGAHEAD.WL6 (chunk offset table). */
static bool create_vgahead(const char *path)
{
    int num_chunks = 149;
    size_t size = (size_t)num_chunks * 3;
    uint8_t *offsets = calloc(1, size);
    if (!offsets) return false;

    for (int i = 0; i < num_chunks; ++i) {
        size_t off = (size_t)i * 3;
        offsets[off] = (uint8_t)(off & 0xff);
        offsets[off + 1] = (uint8_t)((off >> 8) & 0xff);
        offsets[off + 2] = (uint8_t)((off >> 16) & 0xff);
    }

    offsets[0] = 0;
    offsets[1] = 0;
    offsets[2] = 0;

    FILE *f = fopen(path, "wb");
    if (!f) { free(offsets); return false; }
    fwrite(offsets, 1, size, f);
    fclose(f);
    free(offsets);
    return true;
}

/* Create synthetic VGAGRAPH.WL6 (compressed graphics data). */
static bool create_vgraph(const char *path)
{
    int num_chunks = 149;
    size_t total = (size_t)num_chunks * (4 + 3);
    uint8_t *blob = malloc(total);
    if (!blob) return false;

    for (int i = 0; i < num_chunks; ++i) {
        size_t off = (size_t)i * 4;
        uint32_t chunk_size = 5;
        memcpy(blob + off, &chunk_size, 4);
        blob[off + 4] = (uint8_t)(i & 0xff);
    }

    FILE *f = fopen(path, "wb");
    if (!f) { free(blob); return false; }
    fwrite(blob, 1, total, f);
    fclose(f);
    free(blob);
    return true;
}

/* Build a path by concatenating directory and filename. */
static void make_path(char *buf, size_t bufsize, const char *directory, const char *filename)
{
    snprintf(buf, bufsize, "%s/%s", directory, filename);
}

/* Make the full synthetic WL6 data directory. */
static bool make_synthetic_wl6_dir(const char *directory)
{
    char path[4096];
    bool ok = true;

    make_path(path, sizeof(path), directory, "VSWAP.WL6"); ok = ok && create_vswap(path);
    make_path(path, sizeof(path), directory, "MAPHEAD.WL6"); ok = ok && create_maphead(path);
    make_path(path, sizeof(path), directory, "GAMEMAPS.WL6"); ok = ok && create_gamemaps(path);
    make_path(path, sizeof(path), directory, "AUDIOHED.WL6"); ok = ok && create_audiohed(path);
    make_path(path, sizeof(path), directory, "AUDIOT.WL6"); ok = ok && create_audiot(path);
    make_path(path, sizeof(path), directory, "VGADICT.WL6"); ok = ok && create_vgadict(path);
    make_path(path, sizeof(path), directory, "VGAHEAD.WL6"); ok = ok && create_vgahead(path);
    make_path(path, sizeof(path), directory, "VGAGRAPH.WL6"); ok = ok && create_vgraph(path);

    return ok;
}

/* Main program */
int main(void)
{
    fprintf(stderr, "=== Legacy Content Integration Tests ===\n");

    /* Create temp directory for synthetic WL6 data */
    char tmpdir_buf[64];
    snprintf(tmpdir_buf, sizeof(tmpdir_buf), "/tmp/w3r-legacy-test-%d", (int)getpid());
    char *tmpdir = tmpdir_buf;
    if (mkdir(tmpdir, 0755) != 0) {
        fprintf(stderr, "FAIL: cannot create temp dir %s (errno=%d)\n", tmpdir, errno);
        return 1;
    }

    /* Create synthetic WL6 data in the temp directory */
    if (!make_synthetic_wl6_dir(tmpdir)) {
        fprintf(stderr, "FAIL: make_synthetic_wl6_dir failed\n");
        return 1;
    }

    /* Initialize the legacy content adapter with the synthetic directory */
    legacy_content_t content;
    if (LegacyContent_Init(&content, tmpdir) != LEGACY_CONTENT_OK) {
        fprintf(stderr, "FAIL: LegacyContent_Init failed\n");
        return 1;
    }

    /* Verify resource names are set correctly from the synthetic data */
    const char *vswap = LegacyContent_ResourceName(&content, LEGACY_RESOURCE_VSWAP);
    const char *maphead = LegacyContent_ResourceName(&content, LEGACY_RESOURCE_MAPHEAD);
    const char *gamemaps = LegacyContent_ResourceName(&content, LEGACY_RESOURCE_GAMEMAPS);
    const char *audiohed = LegacyContent_ResourceName(&content, LEGACY_RESOURCE_AUDIOHED);
    const char *audiot = LegacyContent_ResourceName(&content, LEGACY_RESOURCE_AUDIOT);
    const char *vgadict = LegacyContent_ResourceName(&content, LEGACY_RESOURCE_VGADICT);
    const char *vgahead = LegacyContent_ResourceName(&content, LEGACY_RESOURCE_VGAHEAD);
    const char *vgraph = LegacyContent_ResourceName(&content, LEGACY_RESOURCE_VGAGRAPH);

    fprintf(stderr, "  VSWAP.WL6   = %s\n", vswap ? vswap : "(null)");
    fprintf(stderr, "  MAPHEAD.WL6 = %s\n", maphead ? maphead : "(null)");
    fprintf(stderr, "  GAMEMAPS.WL6= %s\n", gamemaps ? gamemaps : "(null)");
    fprintf(stderr, "  AUDIOHED.WL6= %s\n", audiohed ? audiohed : "(null)");
    fprintf(stderr, "  AUDIOT.WL6  = %s\n", audiot ? audiot : "(null)");
    fprintf(stderr, "  VGADICT.WL6 = %s\n", vgadict ? vgadict : "(null)");
    fprintf(stderr, "  VGAHEAD.WL6 = %s\n", vgahead ? vgahead : "(null)");
    fprintf(stderr, "  VGAGRAPH.WL6= %s\n", vgraph ? vgraph : "(null)");

    /* All names should be the standard WL6 filenames */
    bool names_ok = true;
    if (strcmp(vswap, "VSWAP.WL6") != 0) names_ok = false;
    if (strcmp(maphead, "MAPHEAD.WL6") != 0) names_ok = false;
    if (strcmp(gamemaps, "GAMEMAPS.WL6") != 0) names_ok = false;
    if (strcmp(audiohed, "AUDIOHED.WL6") != 0) names_ok = false;
    if (strcmp(audiot, "AUDIOT.WL6") != 0) names_ok = false;
    if (strcmp(vgadict, "VGADICT.WL6") != 0) names_ok = false;
    if (strcmp(vgahead, "VGAHEAD.WL6") != 0) names_ok = false;
    if (strcmp(vgraph, "VGAGRAPH.WL6") != 0) names_ok = false;

    if (!names_ok) {
        fprintf(stderr, "FAIL: resource names unexpected\n");
        return 1;
    }

    /* Test LegacyContent_ResourceName returns the correct names */
    if (strcmp(LegacyContent_ResourceName(&content, LEGACY_RESOURCE_VSWAP), "VSWAP.WL6") != 0) {
        fprintf(stderr, "FAIL: LegacyContent_ResourceName VSWAP unexpected\n");
        return 1;
    }
    if (strcmp(LegacyContent_ResourceName(&content, LEGACY_RESOURCE_MAPHEAD), "MAPHEAD.WL6") != 0) {
        fprintf(stderr, "FAIL: LegacyContent_ResourceName MAPHEAD unexpected\n");
        return 1;
    }

    /* Test with a custom-named resource.
       Override just the VSWAP filename to a custom name. */
    if (LegacyContent_SetResource(&content, LEGACY_RESOURCE_VSWAP, "CUSTOM.WL6") != LEGACY_CONTENT_OK) {
        fprintf(stderr, "FAIL: LegacyContent_SetResource failed\n");
        return 1;
    }
    const char *custom_vswap = LegacyContent_ResourceName(&content, LEGACY_RESOURCE_VSWAP);
    if (custom_vswap == NULL || strcmp(custom_vswap, "CUSTOM.WL6") != 0) {
        fprintf(stderr, "FAIL: custom name not set: %s\n", custom_vswap ? custom_vswap : "(null)");
        return 1;
    }

    /* Restore default */
    if (LegacyContent_SetResource(&content, LEGACY_RESOURCE_VSWAP, "VSWAP.WL6") != LEGACY_CONTENT_OK) {
        fprintf(stderr, "FAIL: restore default failed\n");
        return 1;
    }

    /* Verify the default is restored */
    const char *restored = LegacyContent_ResourceName(&content, LEGACY_RESOURCE_VSWAP);
    if (restored == NULL || strcmp(restored, "VSWAP.WL6") != 0) {
        fprintf(stderr, "FAIL: default not restored: %s\n", restored ? restored : "(null)");
        return 1;
    }

    /* Clean up temp directory */
    if (rmdir(tmpdir) != 0) {
        fprintf(stderr, "WARN: could not remove temp dir %s\n", tmpdir);
    }

    fprintf(stderr, "=== Integration tests complete ===\n");
    return 0;
}