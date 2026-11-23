/* SPDX-License-Identifier: GPL-2.0-only */
/* Legacy WL6 content source selection.
   The original game archives keep their WL6 formats and loaders; this
   interface only makes the source directory and file names explicit.
   The type is plain value state: it owns no pointers, can be copied, and
   does not require any asset to exist until validation is requested. */
#ifndef LEGACY_CONTENT_H
#define LEGACY_CONTENT_H

#include <stdbool.h>
#include <stddef.h>

#define LEGACY_CONTENT_DIRECTORY_MAX 4096
#define LEGACY_CONTENT_FILENAME_MAX 64
#define LEGACY_CONTENT_PATH_MAX 4096

typedef enum {
    LEGACY_RESOURCE_VSWAP = 0,
    LEGACY_RESOURCE_MAPHEAD,
    LEGACY_RESOURCE_GAMEMAPS,
    LEGACY_RESOURCE_AUDIOHED,
    LEGACY_RESOURCE_AUDIOT,
    LEGACY_RESOURCE_VGADICT,
    LEGACY_RESOURCE_VGAHEAD,
    LEGACY_RESOURCE_VGAGRAPH,
    LEGACY_RESOURCE_COUNT
} legacy_resource_t;

typedef enum {
    LEGACY_CONTENT_OK = 0,
    LEGACY_CONTENT_NULL,
    LEGACY_CONTENT_EMPTY_DIRECTORY,
    LEGACY_CONTENT_DIRECTORY_TOO_LONG,
    LEGACY_CONTENT_INVALID_DIRECTORY,
    LEGACY_CONTENT_INVALID_RESOURCE,
    LEGACY_CONTENT_EMPTY_FILENAME,
    LEGACY_CONTENT_FILENAME_TOO_LONG,
    LEGACY_CONTENT_INVALID_FILENAME,
    LEGACY_CONTENT_PATH_TOO_LONG,
    LEGACY_CONTENT_MISSING_ASSET
} legacy_content_error_t;

typedef struct {
    char directory[LEGACY_CONTENT_DIRECTORY_MAX];
    char filenames[LEGACY_RESOURCE_COUNT][LEGACY_CONTENT_FILENAME_MAX];
} legacy_content_t;

/* Immutable WL6 filename template. Its directory is empty, so it is not a
   complete configuration until LegacyContent_Init binds a directory. */
const legacy_content_t *LegacyContent_Default(void);

/* Clears and initialises a caller-owned descriptor with the default WL6
   filenames and the supplied directory. No asset access is performed. */
legacy_content_error_t LegacyContent_Init(legacy_content_t *content, const char *directory);

/* Replaces one resource filename. Names are plain file names, not paths. */
legacy_content_error_t LegacyContent_SetResource(legacy_content_t *content,
                                                 legacy_resource_t resource,
                                                 const char *filename);

/* Borrowed name owned by the descriptor, or NULL for invalid input. */
const char *LegacyContent_ResourceName(const legacy_content_t *content,
                                       legacy_resource_t resource);

/* Structural validation only; assets may be absent. */
legacy_content_error_t LegacyContent_Validate(const legacy_content_t *content);

/* Structural validation plus presence and readability of every resource.
   On a missing asset, *missing (when non-NULL) identifies the resource;
   otherwise it is set to LEGACY_RESOURCE_COUNT. */
legacy_content_error_t LegacyContent_ValidateAssets(const legacy_content_t *content,
                                                    legacy_resource_t *missing);

const char *LegacyContent_ErrorString(legacy_content_error_t error);

#endif
