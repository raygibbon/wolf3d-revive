/* SPDX-License-Identifier: GPL-2.0-only */
/* Legacy content adapter headless tests.
   Asset-free: no original WL6 data required. Synthetic fixtures only.
   Exercises the full adapter validation surface used by the production loader.
   Compile with: gcc -I src -I src/engine tests/legacy_content_tests.c src/engine/legacy_content.c src/platform/data_file.c
*/
#include "legacy_content.h"
#include "platform/data_file.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define CHECK(cond) do { if (!(cond)) { fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); return 1; } } while (0)

int main(void)
{
    static const legacy_resource_t default_ids[8] = {
        LEGACY_RESOURCE_VSWAP, LEGACY_RESOURCE_MAPHEAD, LEGACY_RESOURCE_GAMEMAPS,
        LEGACY_RESOURCE_AUDIOHED, LEGACY_RESOURCE_AUDIOT, LEGACY_RESOURCE_VGADICT,
        LEGACY_RESOURCE_VGAHEAD, LEGACY_RESOURCE_VGAGRAPH,
    };
    static const char *const default_names[8] = {
        "VSWAP.WL6", "MAPHEAD.WL6", "GAMEMAPS.WL6", "AUDIOHED.WL6", "AUDIOT.WL6",
        "VGADICT.WL6", "VGAHEAD.WL6", "VGAGRAPH.WL6",
    };

    fprintf(stderr, "=== Legacy Content Adapter Tests (asset-free) ===\n");

    /* Default WL6 resource names resolve through the default descriptor and
       through the NULL-content fallback used before startup. */
    fprintf(stderr, "test_default_resource_names: ");
    { const legacy_content_t *def = LegacyContent_Default();
      for (unsigned i = 0; i < 8; ++i) {
        const char *got = LegacyContent_ResourceName(def, default_ids[i]);
        if (got == NULL || strcmp(got, default_names[i]) != 0) { fprintf(stderr, "FAIL default name %u\n", i); return 1; }
        got = LegacyContent_ResourceName(NULL, default_ids[i]);
        if (got == NULL || strcmp(got, default_names[i]) != 0) { fprintf(stderr, "FAIL default name (null content) %u\n", i); return 1; }
      } }
    fprintf(stderr, "PASS\n");

    /* Directory binding and structural validation. */
    fprintf(stderr, "test_init_binds_directory: ");
    { legacy_content_t content;
      legacy_content_error_t err = LegacyContent_Init(&content, "/tmp/w3r-legacy-unit");
      if (err != LEGACY_CONTENT_OK) { fprintf(stderr, "FAIL valid directory: %d\n", (int)err); return 1; }
      err = LegacyContent_Validate(&content);
      if (err != LEGACY_CONTENT_OK) { fprintf(stderr, "FAIL valid directory validate: %d\n", (int)err); return 1; }
      err = LegacyContent_Init(&content, "");
      if (err != LEGACY_CONTENT_EMPTY_DIRECTORY) { fprintf(stderr, "FAIL empty directory: %d\n", (int)err); return 1; } }
    fprintf(stderr, "PASS\n");

    /* Filename selection, restoration and rejection rules. */
    fprintf(stderr, "test_set_resource: ");
    { legacy_content_t content;
      legacy_content_error_t err = LegacyContent_Init(&content, "/tmp/w3r-legacy-unit");
      if (err != LEGACY_CONTENT_OK) { fprintf(stderr, "FAIL init: %d\n", (int)err); return 1; }
      if (LegacyContent_SetResource(&content, LEGACY_RESOURCE_VSWAP, "CUSTOM.WL6") != LEGACY_CONTENT_OK) { fprintf(stderr, "FAIL set: %d\n", (int)LegacyContent_SetResource(&content, LEGACY_RESOURCE_VSWAP, "CUSTOM.WL6")); return 1; }
      if (LegacyContent_ResourceName(&content, LEGACY_RESOURCE_VSWAP) == NULL || strcmp(LegacyContent_ResourceName(&content, LEGACY_RESOURCE_VSWAP), "CUSTOM.WL6") != 0) { fprintf(stderr, "FAIL set get\n"); return 1; }
      if (LegacyContent_SetResource(&content, LEGACY_RESOURCE_VSWAP, "VSWAP.WL6") != LEGACY_CONTENT_OK) { fprintf(stderr, "FAIL restore\n"); return 1; }
      if (strcmp(LegacyContent_ResourceName(&content, LEGACY_RESOURCE_VSWAP), "VSWAP.WL6") != 0) { fprintf(stderr, "FAIL restore get\n"); return 1; }
      if (LegacyContent_SetResource(&content, (legacy_resource_t)99, "X.WL6") != LEGACY_CONTENT_INVALID_RESOURCE) { fprintf(stderr, "FAIL invalid resource\n"); return 1; }
      if (LegacyContent_SetResource(&content, LEGACY_RESOURCE_VSWAP, "") != LEGACY_CONTENT_EMPTY_FILENAME) { fprintf(stderr, "FAIL empty filename\n"); return 1; }
      if (LegacyContent_SetResource(&content, LEGACY_RESOURCE_VSWAP, "../VGAGRAPH.WL6") != LEGACY_CONTENT_INVALID_FILENAME) { fprintf(stderr, "FAIL traversal\n"); return 1; }
      char long_name[80];
      memset(long_name, 'a', sizeof(long_name) - 1);
      long_name[sizeof(long_name) - 1] = '\0';
      if (strlen(long_name) >= LEGACY_CONTENT_FILENAME_MAX) {
          if (LegacyContent_SetResource(&content, LEGACY_RESOURCE_VSWAP, long_name) != LEGACY_CONTENT_FILENAME_TOO_LONG) { fprintf(stderr, "FAIL overlong filename\n"); return 1; }
      } }
    fprintf(stderr, "PASS\n");

    /* NULL content and error strings. */
    fprintf(stderr, "test_validate_null: ");
    { legacy_content_error_t err = LegacyContent_Validate(NULL);
      if (err == LEGACY_CONTENT_OK) { fprintf(stderr, "FAIL null validate\n"); return 1; } }
    fprintf(stderr, "PASS\n");
    fprintf(stderr, "test_error_strings: ");
    { const char *s1 = LegacyContent_ErrorString(LEGACY_CONTENT_MISSING_ASSET);
      const char *s2 = LegacyContent_ErrorString(LEGACY_CONTENT_INVALID_FILENAME);
      const char *s3 = LegacyContent_ErrorString(LEGACY_CONTENT_OK);
      if (s1 == NULL || s2 == NULL || s3 == NULL) { fprintf(stderr, "FAIL error strings\n"); return 1; } }
    fprintf(stderr, "PASS\n");

    /* Explicit missing-asset reporting. */
    fprintf(stderr, "test_validate_assets_missing: ");
    { mkdir("/tmp/w3r-legacy-unit", 0755);
      static const char *const asset_names[8] = {
          "VSWAP.WL6", "MAPHEAD.WL6", "GAMEMAPS.WL6", "AUDIOHED.WL6", "AUDIOT.WL6",
          "VGADICT.WL6", "VGAHEAD.WL6", "VGAGRAPH.WL6",
      };
      for (unsigned i = 0; i < 8; ++i) {
          char path[512];
          snprintf(path, sizeof(path), "/tmp/w3r-legacy-unit/%s", asset_names[i]);
          FILE *f = fopen(path, "wb");
          if (!f) { fprintf(stderr, "FAIL cannot create fixture %s\n", asset_names[i]); return 1; }
          if (fwrite("x", 1, 1, f) != (size_t)1) { fprintf(stderr, "FAIL cannot fill fixture %s\n", asset_names[i]); return 1; }
          (void)fclose(f);
      }
      { legacy_content_t content;
        legacy_content_error_t err = LegacyContent_Init(&content, "/tmp/w3r-legacy-unit");
        legacy_resource_t missing = LEGACY_RESOURCE_COUNT;
        if (err != LEGACY_CONTENT_OK) { fprintf(stderr, "FAIL init fixture dir\n"); return 1; }
        err = LegacyContent_ValidateAssets(&content, &missing);
        if (err != LEGACY_CONTENT_OK) { fprintf(stderr, "FAIL all-present validation %d\n", (int)err); return 1; }
        if (missing != LEGACY_RESOURCE_COUNT) { fprintf(stderr, "FAIL all-present missing %u\n", (unsigned)missing); return 1; } }
      if (remove("/tmp/w3r-legacy-unit/VGAGRAPH.WL6") != 0) { fprintf(stderr, "FAIL cleanup\n"); return 1; }
      { legacy_content_t content;
        legacy_content_error_t err = LegacyContent_Init(&content, "/tmp/w3r-legacy-unit");
        legacy_resource_t missing = LEGACY_RESOURCE_COUNT;
        err = LegacyContent_ValidateAssets(&content, &missing);
        if (err != LEGACY_CONTENT_MISSING_ASSET) { fprintf(stderr, "FAIL missing asset error %d\n", (int)err); return 1; }
        if (missing != LEGACY_RESOURCE_VGAGRAPH) { fprintf(stderr, "FAIL missing identifier %u\n", (unsigned)missing); return 1; } }
      for (unsigned i = 0; i < 8; ++i) {
          char path[512];
          if (i == 7) continue; /* VGAGRAPH.WL6 was already removed for the missing-asset check. */
          snprintf(path, sizeof(path), "/tmp/w3r-legacy-unit/%s", asset_names[i]);
          if (remove(path) != 0) { fprintf(stderr, "FAIL cleanup %s\n", asset_names[i]); return 1; }
      }
      if (rmdir("/tmp/w3r-legacy-unit") != 0) { fprintf(stderr, "FAIL cleanup dir\n"); return 1; } }
    fprintf(stderr, "PASS\n");

    fprintf(stderr, "=== All adapter tests complete ===\n");
    return 0;
}
