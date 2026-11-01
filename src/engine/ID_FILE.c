/* SPDX-License-Identifier: GPL-2.0-only */
/* Portable bounded form of CA_ReadFile/CA_FarRead for original archives. */
#include "ID_FILE.h"
#include "ID_CA.h"
#include "platform/data_file.h"
#include <stdio.h>
bool CA_ReadDataFile(const char *name, memptr *data, size_t *size)
{
    if (!name || !data || !size || *data) return CA_Fail("Invalid archive read request");
    if (!*CA_DataDirectory()) return CA_Fail("Configure a data directory before reading archives");
    FILE *file = CA_OpenDataFile(CA_DataDirectory(),name);
    long length;
    if (!file) return CA_Fail("Cannot open %s",name);
    bool ok = fseek(file,0,SEEK_END)==0 && (length=ftell(file))>0 && length<=32*1024*1024 && fseek(file,0,SEEK_SET)==0;
    if (!ok) { (void)fclose(file); return CA_Fail("Invalid %s file size",name); }
    *size=(size_t)length;
    ok=MM_GetPtr(data,*size);
    if (ok) ok=fread(*data,1,*size,file)==*size;
    (void)fclose(file);
    if (!ok) { if (*data) (void)MM_FreePtr(data); return CA_Fail("Cannot read %s",name); }
    return true;
}
