#pragma once
#include "../types.h"
#include <array>
#include <string>

namespace ar1 {

// Processor State (PSTATE) bits
// We will separate these eventually, but for now we store raw flags
struct PState {
  u8 N : 1; // Negative
  u8 Z : 1; // Zero
  u8 C : 1; // Carry
  u8 V : 1; // Overflow
  // ... other system flags (DAIF, CurrentEL, etc)
  u8 CurrentEL : 2; // Exception Level (0-3)
};

class Registers {
public:
  Registers();
  ~Registers() = default;

  // --- General Purpose Registers (GPR) ---
  // X0 - X30 (64-bit). W0-W30 are just the lower 32 bits of these.
  // X31 is a special case (Zero Register 'XZR' or Stack Pointer 'SP' depending
  // on context), but physically we store 0-30.
  u64 x[31];

  // --- Special Registers ---
  u64 pc; // Program Counter

  // Stack Pointers are banked by Exception Level
  u64 sp_el0; // User Mode Stack Pointer
  u64 sp_el1; // Kernel Mode Stack Pointer

  // Helper to get the currently active SP based on PSTATE.SP
  u64 get_current_sp() const;
  void set_current_sp(u64 value);

  // --- Processor State ---
  PState pstate;

  // --- System Registers (Essential for Boot) ---
  u64 sctlr_el1; // System Control Register (MMU Enable, Cache Enable)
  u64 ttbr0_el1; // Translation Table Base Register 0 (User Space / Low Mem)
  u64 ttbr1_el1; // Translation Table Base Register 1 (Kernel Space / High Mem)
  u64 tcr_el1;   // Translation Control Register
  u64 mpidr_el1; // Multiprocessor Affinity Register (Core ID)
  u64 esr_el1;   // Exception Syndrome Register
  u64 spsr_el1;  // Saved PSTATE (EL1)
  u64 elr_el1;   // Exception Link Register (EL1)

  // Vector Base Address Register (EL1)
  u64 vbar_el1;

  // Generic Timer Registers
  u64 cntpct_el0;    // Physical Count
  u64 cntp_tval_el0; // Timer Value
  u64 cntp_ctl_el0;  // Control Register
  u64 cntp_cval_el0; // Compare Value

  // SIMD & Floating Point (NEON)
  u128 v[32]; // 32x 128-bit registers
  u64 fpcr;   // Floating Point Control Register
  u64 fpsr;   // Floating Point Status Register

  // Debug helper
  void dump() const;

  // PSTATE helpers
  u64 pack_pstate() const;
  void unpack_pstate(u64 val);
};
} // namespace ar1
