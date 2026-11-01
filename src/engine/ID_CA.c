/* SPDX-License-Identifier: GPL-2.0-only */
/* Direct port of the title graphics slice in WOLFSRC/ID_CA.C.
   Original Huffman tree numbering, chunk cache and file formats are retained.
   All disk integers are read explicitly; no far pointers or node fixups. */
#include "ID_CA.h"
#include "ID_VL.h"
#include "ID_MAP.h"
#include "ID_PM.h"
#include "platform/data_file.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#define CA_MAX_FILE ((size_t)32 * 1024 * 1024)
#define CA_MAX_CHUNK ((size_t)4 * 1024 * 1024)
#define SPARSE UINT32_C(0xffffff)

memptr grsegs[NUMCHUNKS];
size_t grsizes[NUMCHUNKS];
pictabletype pictable[NUMPICS];
static huffnode grhuffman[255];
static uint32_t grstarts[NUMCHUNKS + 1];
static FILE *grhandle;
static char error[512], data_directory[4096];
const char *CA_DataDirectory(void) { return data_directory; }

const char *CA_Error(void) { return error; }
bool CA_Fail(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    (void)vsnprintf(error, sizeof(error), format, args);
    va_end(args);
    return false;
}
static uint16_t ReadLE16(const w3r_byte *p)
{
    return (uint16_t)((uint16_t)p[0] | (uint16_t)((uint16_t)p[1] << 8));
}
static uint32_t ReadLE32(const w3r_byte *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static bool ValidateNode(const huffnode table[255], unsigned node, w3r_byte state[255])
{
    uint16_t children[2];
    if (state[node] == 1) return CA_Fail("Cyclic graphics Huffman dictionary");
    if (state[node] == 2) return true;
    state[node] = 1;
    children[0] = table[node].bit0;
    children[1] = table[node].bit1;
    for (unsigned i = 0; i < 2; ++i) {
        if (children[i] > 510) return CA_Fail("Invalid graphics Huffman node %u", (unsigned)children[i]);
        if (children[i] >= 256 && !ValidateNode(table, (unsigned)children[i] - 256, state))
            return false;
    }
    state[node] = 2;
    return true;
}

bool CAL_ValidateHuffman(const huffnode table[255])
{
    w3r_byte state[255] = {0};
    if (!table) return CA_Fail("Missing Huffman dictionary");
    for (unsigned i = 0; i < 255; ++i)
        if (!ValidateNode(table, i, state)) return false;
    return true;
}

bool CAL_HuffExpand(const w3r_byte *source, size_t source_length,
                    w3r_byte *dest, size_t length, const huffnode table[255])
{
    size_t input = 0, output = 0;
    unsigned mask = 1, node = 254, depth = 0;
    if ((!source && source_length) || (!dest && length) || !table)
        return CA_Fail("Invalid Huffman buffers");
    while (output < length) {
        uint16_t code;
        if (input >= source_length) return CA_Fail("Truncated graphics Huffman stream");
        code = (source[input] & mask) ? table[node].bit1 : table[node].bit0;
        mask <<= 1;
        if (mask == 256) { mask = 1; ++input; }
        if (code < 256) {
            dest[output++] = (w3r_byte)code;
            node = 254;
            depth = 0;
        } else {
            if (code > 510 || ++depth > 255) return CA_Fail("Invalid/cyclic graphics Huffman code");
            node = (unsigned)code - 256;
        }
    }
    return true;
}

static FILE *CA_Open(const char *directory, const char *name)
{
    FILE *file = CA_OpenDataFile(directory, name);
    if (!file) (void)CA_Fail("Cannot open %s in '%s' (missing, ambiguous or unreadable; use --data)", name, directory);
    return file;
}

static bool CA_FileSize(FILE *file, const char *name, size_t *size)
{
    long end;
    if (fseek(file, 0, SEEK_END) != 0 || (end = ftell(file)) <= 0 ||
        (unsigned long)end > CA_MAX_FILE || fseek(file, 0, SEEK_SET) != 0)
        return CA_Fail("%s is empty, unreadable or larger than 32 MiB", name);
    *size = (size_t)end;
    return true;
}

static bool CA_Read(FILE *file, void *dest, size_t length)
{
    return fread(dest, 1, length, file) == length || CA_Fail("Truncated/unreadable game-data file");
}

void CA_Shutdown(void)
{
    CA_UncacheMap(); PM_Shutdown(); data_directory[0]=0;
    if (grhandle) { (void)fclose(grhandle); grhandle = NULL; }
    for (int i = 0; i < NUMCHUNKS; ++i) {
        if (grsegs[i]) (void)MM_FreePtr(&grsegs[i]);
        grsizes[i] = 0;
    }
    memset(pictable, 0, sizeof(pictable));
}

static bool CAL_SetupGrFile(const char *directory)
{
    w3r_byte dict[1024], head[(NUMCHUNKS + 1) * 3];
    FILE *file;
    size_t size;
    file = CA_Open(directory, "VGADICT.WL6");
    if (!file) return false;
    bool ok = CA_FileSize(file, "VGADICT.WL6", &size);
    if (ok && size != 1020 && size != 1024) ok = CA_Fail("VGADICT.WL6 must contain 255 or 256 four-byte nodes");
    if (ok) ok = CA_Read(file, dict, size);
    (void)fclose(file);
    if (!ok) return false;
    for (size_t i = 0; i < 255; ++i) {
        grhuffman[i].bit0 = ReadLE16(dict + i * 4);
        grhuffman[i].bit1 = ReadLE16(dict + i * 4 + 2);
    }
    if (!CAL_ValidateHuffman(grhuffman)) return false;
    file = CA_Open(directory, "VGAHEAD.WL6");
    if (!file) return false;
    ok = CA_FileSize(file, "VGAHEAD.WL6", &size);
    if (ok && size != sizeof(head)) ok = CA_Fail("Unsupported WL6 graphics layout: expected 149 chunks (450-byte VGAHEAD)");
    if (ok) ok = CA_Read(file, head, sizeof(head));
    (void)fclose(file);
    if (!ok) return false;
    grhandle = CA_Open(directory, "VGAGRAPH.WL6");
    if (!grhandle || !CA_FileSize(grhandle, "VGAGRAPH.WL6", &size)) return false;
    uint32_t previous = 0;
    for (size_t i = 0; i <= NUMCHUNKS; ++i) {
        uint32_t offset = (uint32_t)head[i * 3] | ((uint32_t)head[i * 3 + 1] << 8) |
                          ((uint32_t)head[i * 3 + 2] << 16);
        grstarts[i] = offset;
        if (offset == SPARSE) continue;
        if (offset < previous || offset > size) return CA_Fail("Invalid/nonmonotonic VGAHEAD offset at chunk %zu", i);
        previous = offset;
    }
    if (grstarts[0] != 0 || grstarts[NUMCHUNKS] != size)
        return CA_Fail("VGAHEAD terminal offset does not match VGAGRAPH size");
    if (!CA_CacheGrChunk(STRUCTPIC)) return false;
    if (grsizes[STRUCTPIC] != NUMPICS * 4) return CA_Fail("Unsupported WL6 picture table size");
    const w3r_byte *raw = grsegs[STRUCTPIC];
    for (size_t i = 0; i < NUMPICS; ++i) {
        pictable[i].width = ReadLE16(raw + i * 4);
        pictable[i].height = ReadLE16(raw + i * 4 + 2);
    }
    return true;
}

bool CA_Startup(const char *data_dir)
{
    static const char *required[] = {"VSWAP.WL6", "GAMEMAPS.WL6", "MAPHEAD.WL6", "AUDIOT.WL6", "AUDIOHED.WL6"};
    CA_Shutdown();
    error[0] = '\0';
    if (!data_dir || !*data_dir) return CA_Fail("Empty data directory; supply --data /path/to/WL6");
    if (strlen(data_dir)>=sizeof(data_directory)) return CA_Fail("Data directory too long");
    /* Graphics startup checks presence; level and audio parsers have separate lifecycles. */
    for (size_t i = 0; i < sizeof(required) / sizeof(required[0]); ++i) {
        size_t size;
        w3r_byte probe;
        FILE *file = CA_Open(data_dir, required[i]);
        if (!file) return false;
        bool ok = CA_FileSize(file, required[i], &size) && CA_Read(file, &probe, 1);
        (void)fclose(file);
        if (!ok) return false;
    }
    if (!CAL_SetupGrFile(data_dir)) { CA_Shutdown(); return false; }
    strcpy(data_directory,data_dir);
    return true;
}

bool CA_CacheGrChunk(int chunk)
{
    memptr compressed = NULL;
    size_t length, expanded;
    int next;
    bool ok;
    if (!grhandle || chunk < 0 || chunk >= NUMCHUNKS)
        return CA_Fail("Invalid graphics cache request %d", chunk);
    if (grsegs[chunk]) { (void)MM_SetPurge(&grsegs[chunk], 0); return true; }
    if (grstarts[chunk] == SPARSE) return CA_Fail("Missing/sparse graphics chunk %d", chunk);
    next = chunk + 1;
    while (next <= NUMCHUNKS && grstarts[next] == SPARSE) ++next;
    if (next > NUMCHUNKS) return CA_Fail("Missing graphics chunk end offset");
    length = (size_t)(grstarts[next] - grstarts[chunk]);
    if (length < 5 || length > CA_MAX_CHUNK) return CA_Fail("Invalid compressed graphics chunk size");
    if (!MM_GetPtr(&compressed, length)) return CA_Fail("Out of memory reading graphics chunk");
    ok = fseek(grhandle, (long)grstarts[chunk], SEEK_SET) == 0 && CA_Read(grhandle, compressed, length);
    if (!ok) { (void)CA_Fail("Cannot read graphics chunk %d", chunk); goto done; }
    const w3r_byte *source = compressed;
    /* Original tile chunks omit the four-byte size field. */
    if (chunk >= STARTTILE8 && chunk < STARTEXTERNS) {
        expanded = NUMTILE8 * 64;
    } else {
        expanded = ReadLE32(source);
        source += 4;
        length -= 4;
    }
    if (!expanded || expanded > CA_MAX_CHUNK) { ok = CA_Fail("Invalid expanded graphics chunk size"); goto done; }
    if (!MM_GetPtr(&grsegs[chunk], expanded)) { ok = CA_Fail("Out of memory caching graphics chunk"); goto done; }
    ok = CAL_HuffExpand(source, length, grsegs[chunk], expanded, grhuffman);
    if (!ok) (void)MM_FreePtr(&grsegs[chunk]);
    else grsizes[chunk] = expanded;
done:
    (void)MM_FreePtr(&compressed);
    return ok;
}

bool CA_CacheScreen(int chunk)
{
    if (chunk < STARTPICS || chunk >= STARTPICS + NUMPICS)
        return CA_Fail("Invalid full-screen picture %d", chunk);
    if (pictable[chunk - STARTPICS].width != W3R_SCREEN_WIDTH ||
        pictable[chunk - STARTPICS].height != W3R_SCREEN_HEIGHT)
        return CA_Fail("Picture %d is not a 320x200 screen", chunk);
    if (!CA_CacheGrChunk(chunk)) return false;
    if (!VL_MemToScreen(grsegs[chunk], grsizes[chunk], W3R_SCREEN_WIDTH, W3R_SCREEN_HEIGHT, 0, 0))
        return CA_Fail("Invalid full-screen graphics chunk length");
    return true;
}
