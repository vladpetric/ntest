#ifndef __FLIPS_H
#define __FLIPS_H

#include "types.h"

void initFlips();
int lastFlipCount(int sq, u64 mover);
#if defined(__GNUC__) && defined(__x86_64__) && !defined(__MINGW32__)
__attribute__((target("default")))
u64 flips(int sq, u64 mover, u64 enemy);
__attribute__((target("bmi2")))
u64 flips(int sq, u64 mover, u64 enemy);
#else
u64 flips(int sq, u64 mover, u64 enemy);
#endif
extern u64 neighbors[64];
extern uint8_t outsides[8][256];
extern uint8_t insides[8][256];
extern int counts[8][256];
extern u64 rowFlips[8][256];
extern u64 colFlips[8][256];
const int nDiagonals = 11;
extern u64 d9Flips[nDiagonals + 1][256];
extern u64 d7Flips[nDiagonals + 1][256];

#endif
