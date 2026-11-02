/* SPDX-License-Identifier: GPL-2.0-only */
#define _POSIX_C_SOURCE 200809L
#include "save_file.h"
#include <errno.h>
#include <string.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/stat.h>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <io.h>
#include <process.h>
static bool Wide(const char *path,wchar_t result[4096])
{ return MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,path,-1,result,4096)>0; }
FILE *SaveFile_Open(const char *path) { wchar_t p[4096];return Wide(path,p)?_wfopen(p,L"rb"):NULL; }
bool SaveFile_Missing(const char *path)
{
    wchar_t p[4096];if(!Wide(path,p)) return false;
    DWORD attr=GetFileAttributesW(p);
    return attr==INVALID_FILE_ATTRIBUTES && GetLastError()==ERROR_FILE_NOT_FOUND;
}
#else
#include <unistd.h>
FILE *SaveFile_Open(const char *path) { return fopen(path,"rb"); }
bool SaveFile_Missing(const char *path) { struct stat st;return lstat(path,&st)!=0 && errno==ENOENT; }
#endif
bool SaveFile_WriteAtomic(const char *path,const void *bytes,size_t length,bool overwrite)
{
    static unsigned sequence;
    char temp[4096];int fd=-1;
    for(unsigned attempt=0;attempt<100;++attempt) {
#ifdef _WIN32
        int n=snprintf(temp,sizeof(temp),"%s.tmp-%lu-%u",path,(unsigned long)_getpid(),++sequence);
        wchar_t wp[4096];
        if(n<0 || (size_t)n>=sizeof(temp) || !Wide(temp,wp)) return false;
        fd=_wopen(wp,_O_CREAT|_O_EXCL|_O_WRONLY|_O_BINARY,_S_IREAD|_S_IWRITE);
#else
        int n=snprintf(temp,sizeof(temp),"%s.tmp-%lu-%u",path,(unsigned long)getpid(),++sequence);
        if(n<0 || (size_t)n>=sizeof(temp)) return false;
        fd=open(temp,O_CREAT|O_EXCL|O_WRONLY,0600);
#endif
        if(fd>=0) break;
        if(errno!=EEXIST) return false;
    }
    if(fd<0) return false;
#ifdef _WIN32
    FILE *file=_fdopen(fd,"wb");
#else
    FILE *file=fdopen(fd,"wb");
#endif
    bool ok=false;
    if(file) {
        ok=fwrite(bytes,1,length,file)==length && fflush(file)==0;
#ifdef _WIN32
        if(ok) ok=_commit(fd)==0;
#else
        if(ok) ok=fsync(fd)==0;
#endif
        if(fclose(file)!=0) ok=false;
    } else {
#ifdef _WIN32
        (void)_close(fd);
#else
        (void)close(fd);
#endif
    }
#ifdef _WIN32
    wchar_t source[4096],dest[4096];
    if(!Wide(temp,source) || !Wide(path,dest)) return false;
    if(ok) ok=MoveFileExW(source,dest,MOVEFILE_WRITE_THROUGH|(overwrite?MOVEFILE_REPLACE_EXISTING:0))!=0;
    if(!ok) (void)DeleteFileW(source);
#else
    if(ok) {
        /* link() supplies atomic create-if-absent when overwrite was not
           approved, including a competing writer appearing after inspection. */
        ok=overwrite ? rename(temp,path)==0 : link(temp,path)==0;
    }
    (void)unlink(temp);
    if(ok) {
        char parent[4096];(void)snprintf(parent,sizeof(parent),"%s",path);
        char *slash=strrchr(parent,'/');if(slash) *slash=0;else (void)strcpy(parent,".");
        int dir=open(*parent?parent:"/",O_RDONLY|O_DIRECTORY);
        if(dir>=0) { (void)fsync(dir);(void)close(dir); }
    }
#endif
    return ok;
}
