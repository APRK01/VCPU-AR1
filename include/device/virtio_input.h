#pragma once
#include "../types.h"
#include <vector>

namespace ar1 {

class VirtIOInput {
public:
  enum Type { KEYBOARD = 1, MOUSE = 2 };

  VirtIOInput(Type type);

  u32 read(u64 addr);
  void write(u64 addr, u32 val);

  // Inject a key code from the Host (SDL)
  void push_key(u32 key);

private:
  u32 magic;
  u32 version;
  u32 device_id;
  u32 vendor_id;
  u32 status;

  std::vector<u32> key_buffer; // Simple buffer
  friend class VirtIOInput;
};

} // namespace ar1
