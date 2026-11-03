/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef WL_HIGHSCORES_H
#define WL_HIGHSCORES_H
#include <stdbool.h>
#include <stdint.h>
#define HIGH_SCORE_COUNT 7
#define HIGH_NAME_MAX 57
typedef struct { char name[HIGH_NAME_MAX+1];uint32_t score;unsigned completed,episode; } high_score;
void HighScores_Reset(void);
const high_score *HighScores_Table(void);
/* Invalid/future files remain untouched; in-memory defaults still work. */
bool HighScores_Load(const char *directory);
bool HighScores_Save(void);
int HighScores_Insert(uint32_t score,unsigned completed,unsigned episode);
bool BeginHighScoreEntry(int rank,uint64_t ticks);
/* 0 pending, 1 finished, -1 drawing failure. */
int HandleHighScoreEntry(uint64_t ticks);
#endif
