#pragma once
#include "../types.h"
#include <vector>

namespace ar1 {

// Simple L1 Instruction Cache
// 4-way set associative, 64-byte blocks, 32KB total
class ICache {
public:
  struct Line {
    bool valid;
    u64 tag;
    u32 data[16]; // 64 bytes
  };

  ICache();

  // Look up an instruction at a virtual address
  // Returns true on hit, filling 'out_inst'
  bool lookup(u64 va, u32 &out_inst);

  // Fill a cache line from the bus
  void fill(u64 va, const u32 *data);

private:
  static constexpr int NUM_SETS = 128; // 32KB / (64 bytes * 4 ways) = 128
  static constexpr int NUM_WAYS = 4;

  Line sets[NUM_SETS][NUM_WAYS];
  u32 lru[NUM_SETS]; // Simple LRU counter
};

} // namespace ar1
