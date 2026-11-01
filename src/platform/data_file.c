/* SPDX-License-Identifier: GPL-2.0-only */
#include "data_file.h"
#include <ctype.h>
#include <stdint.h>
#include <string.h>
#ifdef _WIN32
#include <io.h>
#else
#include <dirent.h>
#endif


static int EqualName(const char *a, const char *b)
{
    while (*a && *b) {
        if (tolower((unsigned char)*a++) != tolower((unsigned char)*b++)) return 0;
    }
    return *a == *b;
}

FILE *CA_OpenDataFile(const char *directory, const char *name)
{
    if (!directory || !name) return NULL;
    char path[4096], match[256] = {0};
    int count = snprintf(path, sizeof(path), "%s/%s", directory, name);
    if (count < 0 || (size_t)count >= sizeof(path)) return NULL;
    FILE *file = fopen(path, "rb");
    if (file) return file;
    unsigned matches = 0;
#ifdef _WIN32
    struct _finddata_t entry;
    count = snprintf(path, sizeof(path), "%s/*", directory);
    if (count < 0 || (size_t)count >= sizeof(path)) return NULL;
    intptr_t search = _findfirst(path, &entry);
    if (search == -1) return NULL;
    do {
        const char *candidate = entry.name;
#else
    DIR *search = opendir(directory);
    if (!search) return NULL;
    struct dirent *entry;
    while ((entry = readdir(search)) != NULL) {
        const char *candidate = entry->d_name;
#endif
        if (EqualName(candidate, name)) {
            ++matches;
            if (strlen(candidate) < sizeof(match)) strcpy(match, candidate);
        }
#ifdef _WIN32
    } while (_findnext(search, &entry) == 0);
    (void)_findclose(search);
#else
    }
    (void)closedir(search);
#endif
    if (matches != 1 || !match[0]) return NULL;
    count = snprintf(path, sizeof(path), "%s/%s", directory, match);
    return count >= 0 && (size_t)count < sizeof(path) ? fopen(path, "rb") : NULL;
}

