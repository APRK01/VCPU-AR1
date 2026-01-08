#pragma once
#include "../types.h"
#include <vector>

namespace ar1 {

// Simple GICv2 Mock
// Distributor: 0x08000000
// CPU Interface: 0x08010000
class GIC {
public:
  GIC();

  // Memory Offsets
  static constexpr u64 DIST_BASE = 0x08000000;
  static constexpr u64 CPU_BASE = 0x08010000;

  // Access Methods
  u32 read(u64 addr);
  void write(u64 addr, u32 val);

  // Signal an interrupt from hardware
  void set_irq(u32 irq_id);

  // Peek at pending IRQ for the Core
  bool has_pending_irq() const;

private:
  u32 dist_ctl;
  u32 cpu_ctl;
  std::vector<u32> group_enable;
  std::vector<u32> pending;
  std::vector<u32> active;
};

} // namespace ar1
