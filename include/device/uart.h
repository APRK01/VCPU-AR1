#pragma once
#include "../types.h"
#include <iostream>

namespace ar1 {

// Simple PL011 UART Mock
// Base Address usually 0x09000000 on ARM Virt machine
class UART {
public:
  UART() = default;

  // Register Offsets
  static constexpr u64 UARTDR = 0x00; // Data Register
  static constexpr u64 UARTFR = 0x18; // Flag Register

  void write(u64 addr, u32 val) {
    u64 offset = addr & 0xFFF;
    if (offset == UARTDR) {
      // Character output
      std::cout << (char)(val & 0xFF) << std::flush;
    }
  }

  u32 read(u64 addr) {
    u64 offset = addr & 0xFFF;
    if (offset == UARTFR) {
      // Return TX empty (bit 7 clear, TXFE=1 is bit 7? No, bit 7 is usually
      // TXFE) Let's just return 0 (Always ready to send)
      return 0;
    }
    return 0;
  }
};

} // namespace ar1
