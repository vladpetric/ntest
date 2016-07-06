// Copyright (c) 2014,2016 Vlad Petric
// All rights reserved.
//
// This file is distributed subject to GNU GPL version 3. See the files
// GPLv3.txt and License.txt in the instructions subdirectory for details.

#ifndef __BIT_EXTRACTOR_H
#define __BIT_EXTRACTOR_H

/*
This header file provides two macros and a function.
EXTRACT_BITS_U64(value, START, COUNT, STEP)
    Given an unsigned 64 bit value, extract COUNT bits, beginning with position START,
    separated by STEP positions. In other words, gather the bits at positions:
    START, START + STEP, START + 2 * STEP ... , START + (COUNT - 1) * STEP

    START, COUNT and STEP must be constants.

EXTRACT_BITS_U32(value, START, COUNT, STEP)
    Same as above, except for a 32 bit unsigned value. 

The algorithm used for the two EXTRACT_BITS macros is known in the board game programming
community as "magic multiplier".

Essentially, the bits can be gather via three basic operations:
    ((value & mask) * multiplier) >> shift.

The & mask only keeps the bits that we're interested in. The multiplier scrambles those bits around,
generating the sequence that we're interested in as the highest order bits in the 64 or 32 bit
unsigned integer. The right shift brings the sequence down to the lowest order bits.

My own description of the algorithm can be found here:
http://drpetric.blogspot.com/2013/09/bit-gathering-via-multiplication.html

Static Checks:
    START + (COUNT - 1) * STEP should not exceed 64 or 32 bits, respectively (sanity check)
    STEP >= COUNT - this is a limitation of the magic multiplier algorithm

These macros are highly useful for extracting columns or diagonals from bitboards.

The implementation relies on C++ template metaprogramming.

The function - extract_second_diagonal - uses the reverse extraction technique
for the second diagonal. The second diagonal (NESW) has a count of 8 and step
of 7, and thus cannot be handled by the EXTRACT_BITS_U64.
*/

#include <inttypes.h>
template <typename IntT, unsigned int start_bit,
          unsigned int count, unsigned int step>
class meta_repeated_bit {
    static_assert(start_bit + (count - 1) * step <= sizeof(IntT) * 8, "step pattern does not fit 64 bits");
public:
    static const IntT value =
        meta_repeated_bit<IntT, start_bit + step, count - 1, step>::value
        | (IntT(1) << start_bit);
};

template <typename IntT, unsigned int start_bit, unsigned int step>
class meta_repeated_bit <IntT, start_bit, 0, step>
{
public:
    static const IntT value = 0;
};


template <typename IntT, unsigned int start_bit, unsigned int count, unsigned int step>
inline IntT extractor(IntT input) {
    static_assert(count <= step, "count <= step");
    return ((input & meta_repeated_bit<IntT, start_bit, count, step>::value) *
            meta_repeated_bit<IntT, 0, count, step - 1>::value *
            (IntT(1) << (sizeof(IntT) * 8 - 1 - start_bit - (count - 1) * step))) >> 
            (sizeof(IntT) * 8 - count);
}
#define EXTRACT_BITS_U64(value, START, COUNT, STEP) \
    extractor<uint64_t, (START), (COUNT), (STEP)>(value)

#define EXTRACT_BITS_U32(value, START, COUNT, STEP) \
    extractor<uint32_t, (START), (COUNT), (STEP)>(value)

inline uint64_t extract_second_diagonal(uint64_t v) {
    return ((v & meta_repeated_bit<uint64_t, 7, 8, 7>::value) *
            meta_repeated_bit<uint64_t, 0, 8, 8>::value) >> 56;
}

// meta_u32_extractor produces mask, multiplier, and right shift values for
// extracting bits out of a 32 bit unsigned value
template <unsigned start, unsigned count_remain, unsigned step, unsigned count_so_far = 0>
class meta_u32_extractor {
private:
  static constexpr int mult_shift = 32 - start - count_remain;
  static_assert(mult_shift >= 0, "broken");
  static_assert(mult_shift < 32, "broken");
  static constexpr uint32_t single_multiplier = 1ull << mult_shift;
  static constexpr uint32_t remainder_multiplier =
    meta_u32_extractor<start + step, count_remain - 1, step, count_so_far + 1>::multiplier;
  static_assert((single_multiplier & remainder_multiplier) == 0, "broken");
public:
  static constexpr uint32_t multiplier = single_multiplier | remainder_multiplier;
  static constexpr uint32_t and_mask = (1ull << start) | 
    meta_u32_extractor<start + step, count_remain - 1, step, count_so_far + 1>::and_mask;
  static constexpr uint32_t shift = 32 - count_remain; 
};

template <unsigned start, unsigned step, unsigned count_so_far>
class meta_u32_extractor<start, 0u, step, count_so_far> {
public:
  static constexpr uint32_t multiplier = 0;
  static constexpr uint32_t and_mask = 0;
};
#endif  // __BIT_EXTRACTOR_H
