// Copyright 2016 Vlad Petric
//  All Rights Reserved
//    
// This file is distributed subject to GNU GPL version 3. See the files
// GPLv3.txt and License.txt in the instructions subdirectory for details.
//
// 
#pragma once

#include <string>
#include <cinttypes>
namespace Core {
namespace Cache {

union CacheEntryPayload {
    struct __attribute__((packed)) {
    	unsigned lbound: 7;  // score can be from 0 to 64
    	unsigned ubound: 7;
    	unsigned height: 6;
    	unsigned best_move_sq: 6;
    	unsigned iff: 6;
    	unsigned value: 13;
    	unsigned sequence_no: 19;
    } e;
    uint64_t num;
};

static_assert(sizeof(CacheEntryPayload) == sizeof(uint64_t), "CacheEntryPayload is not 64 bits");
} // namespace Cache
} // namespace Core
