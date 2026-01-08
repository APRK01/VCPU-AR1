#pragma once
#include "../types.h"
#include <unordered_map>

namespace ar1 {

// Level 5: Elite Branch Predictor
// Uses a Branch Target Buffer (BTB) and a 2-bit Saturating Counter
class BranchPredictor {
public:
  struct Entry {
    u64 target;
    u8 history; // 2-bit counter: 0,1 (Weak), 2,3 (Strong)
    bool valid;
  };

  BranchPredictor();

  // Predict where the PC should go next
  // Returns 0 if no prediction, otherwise the target PC
  u64 predict(u64 pc);

  // Update the predictor with the actual result
  void update(u64 pc, u64 actual_target, bool taken);

private:
  static constexpr int BTB_SIZE = 1024;
  Entry btb[BTB_SIZE];

  u32 get_index(u64 pc) { return (u32)((pc >> 2) % BTB_SIZE); }
};

} // namespace ar1
