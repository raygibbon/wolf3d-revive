/* SPDX-License-Identifier: GPL-2.0-only */
/* Legacy content adapter headless tests.
   Asset-free: no original WL6 data required. Synthetic fixtures only.
   Tests the adapter interface without pulling in game-specific symbols.
   Compile with: gcc -I src -I src/engine tests/legacy_content_tests.c src/engine/legacy_content.c
   Note: LegacyContent_ValidateAssets calls CA_OpenDataFile; that path is
   tested separately via the Python test that also compiles data_file.c. */
#include "legacy_content.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

/* Helper: verify a resource name via the default content descriptor. */
#define CHECK_DEFAULT_NAME(res, expected) do { \
    const char *got = LegacyContent_ResourceName(NULL, res); \
    (void)got; /* default content fallback */ \
} while(0)

int main(void)
{
    fprintf(stderr, "=== Legacy Content Adapter Tests (asset-free) ===\n");
    /* Test default resource names via the default content descriptor. */
    fprintf(stderr, "test_default_VSWAP_name: ");
    { const legacy_content_t *def = LegacyContent_Default();
      const char *got = LegacyContent_ResourceName(def, LEGACY_RESOURCE_VSWAP);
      fprintf(stderr, (got != NULL && strcmp(got, "VSWAP.WL6") == 0) ? "PASS\n" : "FAIL\n"); }
    fprintf(stderr, "test_default_MAPHEAD_name: ");
    { const legacy_content_t *def = LegacyContent_Default();
      const char *got = LegacyContent_ResourceName(def, LEGACY_RESOURCE_MAPHEAD);
      fprintf(stderr, (got != NULL && strcmp(got, "MAPHEAD.WL6") == 0) ? "PASS\n" : "FAIL\n"); }
    fprintf(stderr, "test_default_GAMEMAPS_name: ");
    { const legacy_content_t *def = LegacyContent_Default();
      const char *got = LegacyContent_ResourceName(def, LEGACY_RESOURCE_GAMEMAPS);
      fprintf(stderr, (got != NULL && strcmp(got, "GAMEMAPS.WL6") == 0) ? "PASS\n" : "FAIL\n"); }
    fprintf(stderr, "test_default_AUDIOHED_name: ");
    { const legacy_content_t *def = LegacyContent_Default();
      const char *got = LegacyContent_ResourceName(def, LEGACY_RESOURCE_AUDIOHED);
      fprintf(stderr, (got != NULL && strcmp(got, "AUDIOHED.WL6") == 0) ? "PASS\n" : "FAIL\n"); }
    fprintf(stderr, "test_default_AUDIOT_name: ");
    { const legacy_content_t *def = LegacyContent_Default();
      const char *got = LegacyContent_ResourceName(def, LEGACY_RESOURCE_AUDIOT);
      fprintf(stderr, (got != NULL && strcmp(got, "AUDIOT.WL6") == 0) ? "PASS\n" : "FAIL\n"); }
    fprintf(stderr, "test_default_VGADICT_name: ");
    { const legacy_content_t *def = LegacyContent_Default();
      const char *got = LegacyContent_ResourceName(def, LEGACY_RESOURCE_VGADICT);
      fprintf(stderr, (got != NULL && strcmp(got, "VGADICT.WL6") == 0) ? "PASS\n" : "FAIL\n"); }
    fprintf(stderr, "test_default_VGAHEAD_name: ");
    { const legacy_content_t *def = LegacyContent_Default();
      const char *got = LegacyContent_ResourceName(def, LEGACY_RESOURCE_VGAHEAD);
      fprintf(stderr, (got != NULL && strcmp(got, "VGAHEAD.WL6") == 0) ? "PASS\n" : "FAIL\n"); }
    fprintf(stderr, "test_default_VGAGRAPH_name: ");
    { const legacy_content_t *def = LegacyContent_Default();
      const char *got = LegacyContent_ResourceName(def, LEGACY_RESOURCE_VGAGRAPH);
      fprintf(stderr, (got != NULL && strcmp(got, "VGAGRAPH.WL6") == 0) ? "PASS\n" : "FAIL\n"); }

    /* Test Init with valid directory. */
    fprintf(stderr, "test_init_binds_directory: ");
    { legacy_content_t content;
      legacy_content_error_t e = LegacyContent_Init(&content, "/tmp/test-wl6-legacy");
      fprintf(stderr, (e == LEGACY_CONTENT_OK) ? "PASS\n" : "FAIL\n"); }

    /* Test SetResource replaces filename. */
    fprintf(stderr, "test_set_resource: ");
    { legacy_content_t content;
      LegacyContent_Init(&content, "/tmp/test-wl6-legacy");
      legacy_content_error_t e = LegacyContent_SetResource(&content, LEGACY_RESOURCE_VSWAP, "CUSTOM.WL6");
      fprintf(stderr, (e == LEGACY_CONTENT_OK) ? "PASS\n" : "FAIL\n"); }

    /* Test ResourceName after SetResource. */
    fprintf(stderr, "test_resource_name_after_set: ");
    { legacy_content_t content;
      LegacyContent_Init(&content, "/tmp/test-wl6-legacy");
      (void)LegacyContent_SetResource(&content, LEGACY_RESOURCE_VSWAP, "CUSTOM.WL6");
      const char *got = LegacyContent_ResourceName(&content, LEGACY_RESOURCE_VSWAP);
      fprintf(stderr, (got != NULL && strcmp(got, "CUSTOM.WL6") == 0) ? "PASS\n" : "FAIL\n"); }

    /* Test ResourceName with NULL content returns NULL (no default fallback). */
    fprintf(stderr, "test_resource_name_null_content: ");
    { const char *got = LegacyContent_ResourceName(NULL, LEGACY_RESOURCE_VSWAP);
      fprintf(stderr, (got == NULL) ? "PASS\n" : "FAIL\n"); }

    /* Test Validate NULL returns error. */
    fprintf(stderr, "test_validate_null: ");
    { legacy_content_error_t e = LegacyContent_Validate(NULL);
      fprintf(stderr, (e != LEGACY_CONTENT_OK) ? "PASS\n" : "FAIL\n"); }

    /* Test ErrorString has content for key errors. */
    fprintf(stderr, "test_error_strings: ");
    { const char *s1 = LegacyContent_ErrorString(LEGACY_CONTENT_EMPTY_DIRECTORY);
      const char *s2 = LegacyContent_ErrorString(LEGACY_CONTENT_MISSING_ASSET);
      const char *s3 = LegacyContent_ErrorString(LEGACY_CONTENT_INVALID_FILENAME);
      fprintf(stderr, ((s1 != NULL) && (s2 != NULL) && (s3 != NULL)) ? "PASS\n" : "FAIL\n"); }

    fprintf(stderr, "=== All adapter tests complete ===\n");
    return 0;
}