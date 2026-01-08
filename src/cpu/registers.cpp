#include "cpu/registers.h"
#include <cstring>
#include <iomanip>
#include <iostream>

namespace ar1 {

Registers::Registers() {
  // Zero out GPRs
  for (int i = 0; i < 31; ++i) {
    x[i] = 0;
  }

  // Initialize Special Registers
  pc = 0;
  sp_el0 = 0;
  sp_el1 = 0;
  vbar_el1 = 0;

  // Initialize System Registers
  sctlr_el1 = 0x00C50030; // Sensible default for ARMv8 (all traps off, MMU off)
  ttbr0_el1 = 0;
  ttbr1_el1 = 0;
  tcr_el1 = 0;
  mpidr_el1 = 0x80000000; // Multi-processor affinity (Core 0)
  esr_el1 = 0;
  elr_el1 = 0;
  spsr_el1 = 0;

  // Init Timer
  cntpct_el0 = 0;
  cntp_tval_el0 = 0;
  cntp_ctl_el0 = 0;
  cntp_cval_el0 = 0;

  // Init FP/NEON
  for (int i = 0; i < 32; ++i) {
    v[i].low = 0;
    v[i].high = 0;
  }
  fpcr = 0;
  fpsr = 0;

  // Default State: EL1 (Kernel Mode)
  pstate.CurrentEL = 1;
  pstate.N = 0;
  pstate.Z = 0;
  pstate.C = 0;
  pstate.V = 0;
}

u64 Registers::get_current_sp() const {
  if (pstate.CurrentEL == 1)
    return sp_el1;
  return sp_el0;
}

void Registers::set_current_sp(u64 value) {
  if (pstate.CurrentEL == 1)
    sp_el1 = value;
  else
    sp_el0 = value;
}

void Registers::unpack_pstate(u64 val) {
  pstate.N = (val >> 31) & 1;
  pstate.Z = (val >> 30) & 1;
  pstate.C = (val >> 29) & 1;
  pstate.V = (val >> 28) & 1;
  pstate.CurrentEL = (val >> 2) & 3;
}

u64 Registers::pack_pstate() const {
  u64 val = 0;
  val |= (u64)pstate.N << 31;
  val |= (u64)pstate.Z << 30;
  val |= (u64)pstate.C << 29;
  val |= (u64)pstate.V << 28;
  val |= (u64)pstate.CurrentEL << 2;
  return val;
}

void Registers::dump() const {
  std::cout << "--- AR1 CPU STATE ---" << std::endl;
  std::cout << "PC  : 0x" << std::setw(16) << std::setfill('0') << std::hex
            << pc << std::endl;
  std::cout << "C_SP: 0x" << std::setw(16) << std::setfill('0') << std::hex
            << get_current_sp() << std::endl;
  std::cout << "EL  : " << std::dec << (int)pstate.CurrentEL << std::endl;
  std::cout << "FLG : [N:" << (int)pstate.N << " Z:" << (int)pstate.Z
            << " C:" << (int)pstate.C << " V:" << (int)pstate.V << "]"
            << std::endl;

  std::cout << "\nGeneral Purpose Registers:" << std::endl;
  for (int i = 0; i < 31; i++) {
    std::cout << "X" << std::setw(2) << std::setfill('0') << std::dec << i
              << ": 0x" << std::setw(16) << std::setfill('0') << std::hex
              << x[i];
    if ((i + 1) % 2 == 0)
      std::cout << std::endl;
    else
      std::cout << "  ";
  }
  std::cout << std::endl;
  std::cout << "SCTLR_EL1: 0x" << std::setw(16) << std::setfill('0') << std::hex
            << sctlr_el1 << "  ";
  std::cout << "TTBR0_EL1: 0x" << std::setw(16) << std::setfill('0') << std::hex
            << ttbr0_el1 << std::endl;
  std::cout << "VBAR_EL1 : 0x" << std::setw(16) << std::setfill('0') << std::hex
            << vbar_el1 << std::endl;
  std::cout << "----------------------" << std::endl;
}

} // namespace ar1
