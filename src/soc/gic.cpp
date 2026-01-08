#include "soc/gic.h"
#include <iostream>

namespace ar1 {

// GICv2 Offsets
// Distributor
#define GICD_CTLR 0x000
#define GICD_TYPER 0x004
#define GICD_ISENABLER 0x100
#define GICD_ICENABLER 0x180

// CPU Interface
#define GICC_CTLR 0x000
#define GICC_PMR 0x004
#define GICC_IAR 0x00C
#define GICC_EOIR 0x010

GIC::GIC() : dist_ctlr(0), cpu_ctlr(0), cpu_pmr(0), pending_irq_count(0) {
  enabled_irqs.resize(1024, false);
  pending_irqs.resize(1024, false);
  active_irqs.resize(1024, false);
}

u32 GIC::read_dist(u32 offset) {
  std::lock_guard<std::mutex> lock(mutex);
  if (offset == GICD_CTLR)
    return dist_ctlr;
  if (offset == GICD_TYPER)
    return 0; // Simplified
  if (offset >= GICD_ISENABLER && offset < GICD_ISENABLER + 0x80) {
    // Read enabled bits
    int idx = (offset - GICD_ISENABLER) / 4;
    u32 val = 0;
    for (int i = 0; i < 32; i++) {
      if (enabled_irqs[idx * 32 + i])
        val |= (1 << i);
    }
    return val;
  }
  return 0;
}

void GIC::write_dist(u32 offset, u32 value) {
  std::lock_guard<std::mutex> lock(mutex);
  if (offset == GICD_CTLR) {
    dist_ctlr = value & 1;
  } else if (offset >= GICD_ISENABLER && offset < GICD_ISENABLER + 0x80) {
    int idx = (offset - GICD_ISENABLER) / 4;
    for (int i = 0; i < 32; i++) {
      if (value & (1 << i))
        enabled_irqs[idx * 32 + i] = true;
    }
  } else if (offset >= GICD_ICENABLER && offset < GICD_ICENABLER + 0x80) {
    int idx = (offset - GICD_ICENABLER) / 4;
    for (int i = 0; i < 32; i++) {
      if (value & (1 << i))
        enabled_irqs[idx * 32 + i] = false;
    }
  }
}

u32 GIC::read_cpu(u32 offset) {
  std::lock_guard<std::mutex> lock(mutex);
  if (offset == GICC_CTLR)
    return cpu_ctlr;
  if (offset == GICC_PMR)
    return cpu_pmr;
  if (offset == GICC_IAR) {
    return acknowledge_irq(); // Side effect: Reads ID
  }
  return 0;
}

void GIC::write_cpu(u32 offset, u32 value) {
  std::lock_guard<std::mutex> lock(mutex);
  if (offset == GICC_CTLR)
    cpu_ctlr = value & 1;
  if (offset == GICC_PMR)
    cpu_pmr = value & 0xFF;
  if (offset == GICC_EOIR) {
    end_of_interrupt(value & 0x3FF);
  }
}

void GIC::set_irq(u32 irq, bool level) {
  std::lock_guard<std::mutex> lock(mutex);
  if (irq >= 1024)
    return;

  // Simple edge/level logic (treating all as level for now)
  if (level && !pending_irqs[irq]) {
    pending_irqs[irq] = true;
    if (enabled_irqs[irq])
      pending_irq_count++;
  }
  // If level drops, do we clear pending? Usually only if not latched.
  // For timer (level sensitive), if the timer condition clears, the IRQ clears.
  if (!level && pending_irqs[irq]) {
    pending_irqs[irq] = false;
    if (enabled_irqs[irq] && pending_irq_count > 0)
      pending_irq_count--;
  }
}

u32 GIC::acknowledge_irq() {
  // Find highest priority pending interrupt
  // For simplicity, just find first pending & enabled
  for (u32 i = 0; i < 1024; i++) {
    if (pending_irqs[i] && enabled_irqs[i]) {
      pending_irqs[i] = false; // Acknowledge consumes pending state
      active_irqs[i] = true;
      if (pending_irq_count > 0)
        pending_irq_count--;
      return i;
    }
  }
  return 1023; // Spurious ID
}

void GIC::end_of_interrupt(u32 irq) {
  if (irq < 1024)
    active_irqs[irq] = false;
}

} // namespace ar1
