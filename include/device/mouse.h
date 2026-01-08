#pragma once
#include "../types.h"
#include <atomic>
#include <mutex>

namespace ar1 {

// Mouse Device Memory Map
// 0x0C000000 - MOUSE_X (read)
// 0x0C000004 - MOUSE_Y (read)
// 0x0C000008 - MOUSE_BUTTONS (read: bit0=left, bit1=right, bit2=middle)
// 0x0C00000C - MOUSE_PRESENT (read: 1 if mouse in window)

constexpr u64 MOUSE_BASE = 0x0C000000;

class Mouse {
public:
  Mouse() = default;

  // Called by Display when mouse moves
  void set_position(i32 x, i32 y) {
    std::lock_guard<std::mutex> lock(mutex);
    mouse_x = x;
    mouse_y = y;
  }

  void set_buttons(u32 buttons) {
    std::lock_guard<std::mutex> lock(mutex);
    mouse_buttons = buttons;
  }

  void set_present(bool present) { mouse_present = present; }

  // MMIO Read
  u32 read(u32 offset) {
    std::lock_guard<std::mutex> lock(mutex);
    switch (offset) {
    case 0x00:
      return (u32)mouse_x;
    case 0x04:
      return (u32)mouse_y;
    case 0x08:
      return mouse_buttons;
    case 0x0C:
      return mouse_present ? 1 : 0;
    default:
      return 0;
    }
  }

  // MMIO Write (no-op for mouse)
  void write(u32 offset, u32 value) {
    (void)offset;
    (void)value;
  }

private:
  i32 mouse_x = 0;
  i32 mouse_y = 0;
  u32 mouse_buttons = 0;
  std::atomic<bool> mouse_present{false};
  std::mutex mutex;
};

} // namespace ar1
