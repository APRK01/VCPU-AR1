#pragma once
#include "../types.h"
#include <mutex>
#include <vector>

namespace ar1 {

class GIC {
public:
  GIC();

  // Register Access
  u32 read_dist(u32 offset);
  void write_dist(u32 offset, u32 value);

  u32 read_cpu(u32 offset);
  void write_cpu(u32 offset, u32 value);

  // Interface for Core/Peripherals
  void set_irq(u32 irq, bool level);
  u32 acknowledge_irq();          // Called when reading IAR
  void end_of_interrupt(u32 irq); // Called when writing EOIR

  bool is_irq_pending() const { return pending_irq_count > 0; }

private:
  mutable std::mutex mutex;

  // Registers
  u32 dist_ctlr;
  u32 cpu_ctlr;
  u32 cpu_pmr;

  // Interrupt State
  std::vector<bool> enabled_irqs;
  std::vector<bool> pending_irqs;
  std::vector<bool> active_irqs;

  u32 pending_irq_count;
};

} // namespace ar1
