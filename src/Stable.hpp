#pragma once
#include <cinttypes>

extern const uint8_t row0_stable[6561];
extern const uint64_t col0_stable[6561];
extern const uint64_t col7_stable[6561];

extern const uint32_t base2_to_base3[256];

const uint64_t AFile = 0x8080808080808080ULL;
const uint64_t FirstRow = 0xFFULL;
const uint64_t LastRow = FirstRow << 56;
const uint64_t HFile = AFile >> 7;
const uint64_t Corners = 0x8100000000000081ULL;
const uint64_t All = 0xFFFFFFFFFFFFFFFFULL;

uint64_t stable_discs(uint64_t mover, uint64_t enemy, uint64_t empty,
                      uint64_t stable = 0ULL);

inline uint64_t stable_next_mask(uint64_t stable, uint64_t occupied) {
    return
        (Corners |
         (stable & ~AFile) << 1 |
         (stable & ~HFile) >> 1 |
         stable >> 8 |  // could mask by ~FirstRow, but shift takes care of it
         stable << 8 |  // similarly, with LastRow
         (stable & ~(HFile | FirstRow)) >> 9 |
         (stable & ~(AFile | LastRow)) << 9 |
         (stable & ~(HFile | LastRow)) << 7 |
         (stable & ~(AFile | FirstRow)) >> 7) & ~occupied;
}
