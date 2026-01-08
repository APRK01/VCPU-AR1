#pragma once
#include "../types.h"
#include <vector>

namespace ar1 {

// Simplified VirtIO-GPU over MMIO
// Base Address: 0x0a000000
class VirtIOGPU {
public:
  VirtIOGPU(u64 fb_base);

  // MMIO Access
  u32 read(u64 addr);
  void write(u64 addr, u32 val);

  // Framebuffer Info
  u32 width;
  u32 height;
  u64 fb_phys_base;

private:
  // Registers
  u32 magic;     // 0x74726976 "virt"
  u32 version;   // 2
  u32 device_id; // 16 (GPU)
  u32 vendor_id; // 0x1234

  u32 status;
  u32 config_generation;
};

} // namespace ar1
