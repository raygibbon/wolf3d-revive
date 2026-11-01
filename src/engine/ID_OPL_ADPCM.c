/* SPDX-License-Identifier: GPL-2.0-only */
/* Compile the pinned MIT dependency once, outside CART's recursive src scan.
   Keep upstream warnings separate from the strict port; see third_party README. */
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wpedantic"
#endif
#include "../../third_party/emu8950/emuadpcm.c"
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
