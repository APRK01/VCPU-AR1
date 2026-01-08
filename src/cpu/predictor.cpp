#include "cpu/predictor.h"

namespace ar1 {

BranchPredictor::BranchPredictor() {
  for (int i = 0; i < BTB_SIZE; i++) {
    btb[i].valid = false;
    btb[i].history = 2; // Start with "Weakly Taken"
    btb[i].target = 0;
  }
}

u64 BranchPredictor::predict(u64 pc) {
  u32 idx = get_index(pc);
  if (btb[idx].valid && btb[idx].history >= 2) {
    return btb[idx].target;
  }
  return 0; // No prediction or "Not Taken"
}

void BranchPredictor::update(u64 pc, u64 actual_target, bool taken) {
  u32 idx = get_index(pc);
  btb[idx].valid = true;
  btb[idx].target = actual_target;

  if (taken) {
    if (btb[idx].history < 3)
      btb[idx].history++;
  } else {
    if (btb[idx].history > 0)
      btb[idx].history--;
  }
}

} // namespace ar1
