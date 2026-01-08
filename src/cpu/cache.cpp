#include "cpu/cache.h"
#include <cstring>

namespace ar1 {

ICache::ICache() {
  for (int s = 0; s < NUM_SETS; s++) {
    lru[s] = 0;
    for (int w = 0; w < NUM_WAYS; w++) {
      sets[s][w].valid = false;
    }
  }
}

bool ICache::lookup(u64 va, u32 &out_inst) {
  u64 tag = va >> 13;             // Higher bits
  u32 set_idx = (va >> 6) & 0x7F; // 7 bits for 128 sets
  u32 offset = (va >> 2) & 0xF;   // 4 bits for 16 instructions

  for (int w = 0; w < NUM_WAYS; w++) {
    if (sets[set_idx][w].valid && sets[set_idx][w].tag == tag) {
      out_inst = sets[set_idx][w].data[offset];
      return true;
    }
  }
  return false;
}

void ICache::fill(u64 va, const u32 *data) {
  u64 tag = va >> 13;
  u32 set_idx = (va >> 6) & 0x7F;

  // Simple Circular LRU
  u32 way = lru[set_idx];
  sets[set_idx][way].valid = true;
  sets[set_idx][way].tag = tag;
  std::memcpy(sets[set_idx][way].data, data, 64);

  lru[set_idx] = (way + 1) % NUM_WAYS;
}

} // namespace ar1
