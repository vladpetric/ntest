#ifndef __CORE_STABLE_H_
#define __CORE_STABLE_H_

extern const uint64_t row0_stable[6561];
extern const uint64_t col0_stable[6561];
extern const uint64_t row7_stable[6561];
extern const uint64_t col7_stable[6561];
uint64_t edge_stable(uint64_t mover, uint64_t enemy);
uint64_t full_stable(uint64_t mover, uint64_t enemy);
#endif  // __CORE_STABLE_H_
