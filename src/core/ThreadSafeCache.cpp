// Copyright 2016 Vlad Petric
//  All Rights Reserved
//    
// This file is distributed subject to GNU GPL version 3. See the files
// GPLv3.txt and License.txt in the instructions subdirectory for details.
//
// 
#include <sys/mman.h>

#include <atomic>
#include <cstdint>
#include <iostream>

#include "core/ThreadSafeCache.h"

namespace Core {
namespace Cache {
struct RawCacheEntry {
  std::atomic<uint64_t> mover_;
  std::atomic<uint64_t> empty_;
  std::atomic<uint64_t> payload_;
  std::atomic<uint64_t> opponent_xor_payload_;
};

class TSCache final {
private:
  TSCache() __attribute__((noinline));
  static constexpr size_t N_ENTRIES = 1ULL << 24;
  static constexpr unsigned ASSOCIATIVITY = 4;
  struct RawCacheEntry* entries_;
  friend TSCache* cache();
};

TSCache::TSCache() {
  char* addr = reinterpret_cast<char *>(
    ::mmap(nullptr,
           sizeof(RawCacheEntry) * N_ENTRIES,
           PROT_READ | PROT_WRITE,
           MAP_HUGETLB | MAP_ANONYMOUS, -1, 0));
  if (addr == MAP_FAILED) {
    ::perror("mmap failed");
    exit(1);
  } else {
    std::cout << "mmap succeeded" << '\n';
  }
  entries_ = reinterpret_cast<RawCacheEntry*>(addr);
}


TSCache* cache() {
  static TSCache c;
  return &c;
}

} // namespace Cache
} // namespace Core
