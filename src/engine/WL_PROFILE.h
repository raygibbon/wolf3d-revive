/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef WL_PROFILE_H
#define WL_PROFILE_H
/* Optional benchmark instrumentation; absent from normal and test builds. */
#ifdef WOLF3D_BENCHMARK
#include <stdint.h>
typedef struct {
    uint64_t rays,sprites,capture,sample,convert,upload,present,empty_queues;
} wl_profile;
extern wl_profile WL_Profile;
uint64_t WL_ProfileNow(void);
#define WL_PROFILE_BEGIN(name) uint64_t name=WL_ProfileNow()
#define WL_PROFILE_END(field,name) (WL_Profile.field+=WL_ProfileNow()-(name))
#else
#define WL_PROFILE_BEGIN(name)
#define WL_PROFILE_END(field,name) ((void)0)
#endif
#endif
