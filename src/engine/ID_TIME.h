/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef ID_TIME_H
#define ID_TIME_H
#include <stdint.h>
/* TickBase is 70 in ID_SD.H. Preserve fractional nanoseconds without drift. */
static inline uint64_t TimeCount_FromNS(uint64_t ns)
{
    return (ns / UINT64_C(1000000000)) * 70 + (ns % UINT64_C(1000000000)) * 70 / UINT64_C(1000000000);
}
#endif
