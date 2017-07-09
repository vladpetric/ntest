#ifndef __FLIPS_H
#define __FLIPS_H

#include "types.h"

void initFlips();
int lastFlipCount(int sq, u64 mover);
__attribute__((target("default")))
u64 flips(int sq, u64 mover, u64 enemy);
__attribute__((target("bmi2")))
u64 flips(int sq, u64 mover, u64 enemy);
extern u64 neighbors[64];

#endif
