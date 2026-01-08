#pragma once
#include "../types.h"
#include <fstream>
#include <string>
#include <vector>

namespace ar1 {

// Standard VirtIO MMIO Registers
struct VirtIORegs {
  u32 magic;           // 0x00
  u32 version;         // 0x04
  u32 device_id;       // 0x08
  u32 vendor_id;       // 0x0c
  u32 device_features; // 0x10
  u32 status;          // 0x70
};

class VirtIOBlock {
public:
  VirtIOBlock(const std::string &img_path);
  ~VirtIOBlock();

  u32 read(u64 addr);
  void write(u64 addr, u32 val);

private:
  VirtIORegs regs;
  std::ifstream image;
  u64 capacity; // sectors
};

} // namespace ar1
