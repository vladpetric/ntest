// Copyright (c) 2016 Vlad Petric
// All rights reserved.
//
// This file is distributed subject to GNU GPL version 3. See the files
// GPLv3.txt and License.txt in the instructions subdirectory for details.

#pragma once
#include <cinttypes>
#include <limits>
#include <stdexcept>

/* This header file provides a single extract-and-convert-to-base3 function,
 * similar to the ones in n64/bitextractor.h, but performing base3 conversion
 * as well.
 *
 * Notes:
 * 
 * 1. Not all patterns that can be bit-extracted can be bit-extract-and-converted.
 * The reason is bit aliasing - 3**0 + 3**1 + 3**2 + ... + 3**5 is 365, taking
 * 9 bits. The base 2 numbers only take 6 bits, obviously.
 *
 * 2. The returned multiplier places the desired result in the low order bits of the
 * high u32 of the result. A mask of STEP low-order bits on the high part of the result
 * should be used to clean up the result.
 *
 * extract_base3_u32_hilo_multiplier(START, COUNT, STEP)
 */

constexpr uint32_t pow3_u32(unsigned n) {
  return (n <= 20 ? (n == 0? 1 : pow3_u32(n - 1) * 3) :
                    throw std::logic_error("max n is 20"));
}

constexpr uint64_t pow3_u64(unsigned n) {
  return (n <= 40 ? (n == 0? 1 : pow3_u32(n - 1) * 3) :
                    throw std::logic_error("max n is 40"));
}

template<unsigned start, unsigned count_remain, unsigned step, unsigned count_so_far = 0>
struct meta_base3_u32_hilo {
  static_assert(start > 0, "meta_base3_u32_hilo cannot capture the first bit");
  static_assert(start + step * (count_remain - 1) < 32, "meta_base3_u32_hilo limited to 32 bits");
  static constexpr uint32_t pow3_factor = pow3_u32(count_so_far);
  static constexpr uint32_t shift = 1u << (32 - start);
  static constexpr uint64_t single_multiplier_64 = pow3_factor * shift;
  static_assert(single_multiplier_64 <= std::numeric_limits<uint32_t>::max(),
                "multiplier exceeds 32 bits");
  static constexpr uint32_t single_multiplier
    = static_cast<uint32_t>(single_multiplier_64);

  static constexpr uint32_t remainder_multiplier =
    meta_base3_u32_hilo<start + step, count_remain - 1,
                       step, count_so_far + 1>::multiplier;
  static_assert((single_multiplier & remainder_multiplier) == 0, "broken");
  static constexpr uint32_t multiplier = single_multiplier | remainder_multiplier;
};

template<unsigned start, unsigned step, unsigned count_so_far>
struct meta_base3_u32_hilo<start, 0, step, count_so_far> {
  static constexpr uint32_t multiplier = 0;
};
