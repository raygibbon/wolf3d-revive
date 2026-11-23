/* SPDX-License-Identifier: GPL-2.0-only */
#include "legacy_content.h"
#include "platform/data_file.h"

#include <stdio.h>
#include <string.h>

static const char *const default_names[LEGACY_RESOURCE_COUNT] = {
    [LEGACY_RESOURCE_VSWAP] = "VSWAP.WL6",
    [LEGACY_RESOURCE_MAPHEAD] = "MAPHEAD.WL6",
    [LEGACY_RESOURCE_GAMEMAPS] = "GAMEMAPS.WL6",
    [LEGACY_RESOURCE_AUDIOHED] = "AUDIOHED.WL6",
    [LEGACY_RESOURCE_AUDIOT] = "AUDIOT.WL6",
    [LEGACY_RESOURCE_VGADICT] = "VGADICT.WL6",
    [LEGACY_RESOURCE_VGAHEAD] = "VGAHEAD.WL6",
    [LEGACY_RESOURCE_VGAGRAPH] = "VGAGRAPH.WL6",
};

static const legacy_content_t default_content = {
    .directory = "",
    .filenames = {
        [LEGACY_RESOURCE_VSWAP] = "VSWAP.WL6",
        [LEGACY_RESOURCE_MAPHEAD] = "MAPHEAD.WL6",
        [LEGACY_RESOURCE_GAMEMAPS] = "GAMEMAPS.WL6",
        [LEGACY_RESOURCE_AUDIOHED] = "AUDIOHED.WL6",
        [LEGACY_RESOURCE_AUDIOT] = "AUDIOT.WL6",
        [LEGACY_RESOURCE_VGADICT] = "VGADICT.WL6",
        [LEGACY_RESOURCE_VGAHEAD] = "VGAHEAD.WL6",
        [LEGACY_RESOURCE_VGAGRAPH] = "VGAGRAPH.WL6",
    },
};

static bool ValidResource(legacy_resource_t resource)
{
    return (unsigned)resource < (unsigned)LEGACY_RESOURCE_COUNT;
}

/* Bounded read: true only when buffer holds a NUL terminator within capacity,
   so descriptor-owned arrays are never read past their field storage. */
static bool Terminated(const char *buffer, size_t capacity)
{
    if (!buffer) return false;
    for (size_t i = 0; i < capacity; ++i) {
        if (buffer[i] == '\0') return true;
    }
    return false;
}

static legacy_content_error_t ValidateDirectory(const char *directory)
{
    size_t length;
    if (!directory || !*directory) return LEGACY_CONTENT_EMPTY_DIRECTORY;
    length = strlen(directory);
    if (length >= LEGACY_CONTENT_DIRECTORY_MAX) return LEGACY_CONTENT_DIRECTORY_TOO_LONG;
    for (size_t i = 0; i < length; ++i) {
        unsigned char c = (unsigned char)directory[i];
        if (c < 0x20U || c == 0x7fU) return LEGACY_CONTENT_INVALID_DIRECTORY;
    }
    return LEGACY_CONTENT_OK;
}

static legacy_content_error_t ValidateFilename(const char *filename)
{
    size_t length;
    if (!filename || !*filename) return LEGACY_CONTENT_EMPTY_FILENAME;
    length = strlen(filename);
    if (length >= LEGACY_CONTENT_FILENAME_MAX) return LEGACY_CONTENT_FILENAME_TOO_LONG;
    for (size_t i = 0; i < length; ++i) {
        unsigned char c = (unsigned char)filename[i];
        if (c < 0x20U || c == 0x7fU || c == '/' || c == '\\' || c == ':')
            return LEGACY_CONTENT_INVALID_FILENAME;
    }
    if (strstr(filename, "..")) return LEGACY_CONTENT_INVALID_FILENAME;
    return LEGACY_CONTENT_OK;
}

const legacy_content_t *LegacyContent_Default(void)
{
    return &default_content;
}

legacy_content_error_t LegacyContent_Init(legacy_content_t *content, const char *directory)
{
    legacy_content_error_t error;
    if (!content) return LEGACY_CONTENT_NULL;
    memset(content, 0, sizeof(*content));
    error = ValidateDirectory(directory);
    if (error != LEGACY_CONTENT_OK) return error;
    memcpy(content->directory, directory, strlen(directory) + 1);
    for (size_t i = 0; i < LEGACY_RESOURCE_COUNT; ++i) {
        size_t length = strlen(default_names[i]);
        memcpy(content->filenames[i], default_names[i], length + 1);
    }
    return LEGACY_CONTENT_OK;
}

legacy_content_error_t LegacyContent_SetResource(legacy_content_t *content,
                                                 legacy_resource_t resource,
                                                 const char *filename)
{
    legacy_content_error_t error;
    if (!content) return LEGACY_CONTENT_NULL;
    if (!ValidResource(resource)) return LEGACY_CONTENT_INVALID_RESOURCE;
    error = ValidateFilename(filename);
    if (error != LEGACY_CONTENT_OK) return error;
    memcpy(content->filenames[resource], filename, strlen(filename) + 1);
    return LEGACY_CONTENT_OK;
}

const char *LegacyContent_ResourceName(const legacy_content_t *content,
                                       legacy_resource_t resource)
{
    if (!ValidResource(resource)) return NULL;
    /* No descriptor bound yet: resolve through the immutable WL6 defaults so
       gradual adoption keeps legacy behaviour. */
    if (!content) return default_names[resource];
    return content->filenames[resource];
}

legacy_content_error_t LegacyContent_Validate(const legacy_content_t *content)
{
    size_t directory_length;
    legacy_content_error_t error;
    if (!content) return LEGACY_CONTENT_NULL;
    /* Descriptor-owned arrays must be NUL-terminated within their capacity;
       reject malformed storage without reading past the array bounds. */
    if (!Terminated(content->directory, sizeof(content->directory)))
        return LEGACY_CONTENT_INVALID_DIRECTORY;
    error = ValidateDirectory(content->directory);
    if (error != LEGACY_CONTENT_OK) return error;
    directory_length = strlen(content->directory);
    for (size_t i = 0; i < LEGACY_RESOURCE_COUNT; ++i) {
        const char *filename = content->filenames[i];
        if (!Terminated(filename, sizeof(content->filenames[i])))
            return LEGACY_CONTENT_INVALID_FILENAME;
        error = ValidateFilename(filename);
        if (error != LEGACY_CONTENT_OK) return error;
        if (directory_length + 1 + strlen(filename) >= LEGACY_CONTENT_PATH_MAX)
            return LEGACY_CONTENT_PATH_TOO_LONG;
    }
    return LEGACY_CONTENT_OK;
}

legacy_content_error_t LegacyContent_ValidateAssets(const legacy_content_t *content,
                                                    legacy_resource_t *missing)
{
    legacy_content_error_t error;
    if (missing) *missing = LEGACY_RESOURCE_COUNT;
    error = LegacyContent_Validate(content);
    if (error != LEGACY_CONTENT_OK) return error;
    for (size_t i = 0; i < LEGACY_RESOURCE_COUNT; ++i) {
        const char *filename = content->filenames[i];
        FILE *file = CA_OpenDataFile(content->directory, filename);
        bool readable = false;
        if (file) {
            readable = fseek(file, 0, SEEK_END) == 0 && ftell(file) > 0;
            (void)fclose(file);
        }
        if (!readable) {
            if (missing) *missing = (legacy_resource_t)i;
            return LEGACY_CONTENT_MISSING_ASSET;
        }
    }
    return LEGACY_CONTENT_OK;
}

const char *LegacyContent_ErrorString(legacy_content_error_t error)
{
    switch (error) {
    case LEGACY_CONTENT_OK: return "no error";
    case LEGACY_CONTENT_NULL: return "legacy content is NULL";
    case LEGACY_CONTENT_EMPTY_DIRECTORY: return "legacy content directory is empty";
    case LEGACY_CONTENT_DIRECTORY_TOO_LONG: return "legacy content directory is too long";
    case LEGACY_CONTENT_INVALID_DIRECTORY:
        return "legacy content directory contains control characters";
    case LEGACY_CONTENT_INVALID_RESOURCE: return "invalid legacy resource identifier";
    case LEGACY_CONTENT_EMPTY_FILENAME: return "legacy resource filename is empty";
    case LEGACY_CONTENT_FILENAME_TOO_LONG: return "legacy resource filename is too long";
    case LEGACY_CONTENT_INVALID_FILENAME:
        return "legacy resource filename must be a plain file name";
    case LEGACY_CONTENT_PATH_TOO_LONG: return "legacy content path exceeds the platform limit";
    case LEGACY_CONTENT_MISSING_ASSET: return "legacy content asset is missing, empty or unreadable";
    }
    return "unknown legacy content error";
}
