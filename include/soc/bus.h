#pragma once
#include "../device/framebuffer.h"
#include "../device/mouse.h"
#include "../device/network.h"
#include "../device/uart.h"
#include "../device/virtio_blk.h"
#include "../types.h"
#include "gic.h"
#include "ram.h"
#include <memory>

namespace ar1 {

constexpr u64 GIC_DIST_BASE = 0x08000000;
constexpr u64 GIC_CPU_BASE = 0x08100000;
constexpr u64 UART_BASE = 0x09000000;
constexpr u64 VIRTIO_BASE = 0x0A000000;
constexpr u64 FB_BASE = 0x0B000000;         // Framebuffer MMIO
constexpr u64 MOUSE_BASE_ADDR = 0x0C000000; // Mouse MMIO
constexpr u64 NET_BASE_ADDR = 0x0D000000;   // Network MMIO
constexpr u64 RAM_BASE = 0x40000000;
constexpr u64 RAM_SIZE = 64ULL * 1024 * 1024;
constexpr u64 VRAM_BASE = 0xc0000000;
constexpr u64 VRAM_SIZE = 8ULL * 1024 * 1024;

class Bus {
public:
  Bus();
  ~Bus() = default;

  std::shared_ptr<RAM> ram;
  std::shared_ptr<RAM> vram;
  std::shared_ptr<UART> uart;
  std::shared_ptr<GIC> gic;
  std::shared_ptr<VirtIOBlock> virtio_blk;
  std::shared_ptr<Framebuffer> framebuffer;
  std::shared_ptr<Mouse> mouse;
  std::shared_ptr<Network> network;

  u8 read8(u64 addr);
  u16 read16(u64 addr);
  u32 read32(u64 addr);
  u64 read64(u64 addr);

  void write8(u64 addr, u8 value);
  void write16(u64 addr, u16 value);
  void write32(u64 addr, u32 value);
  void write64(u64 addr, u64 value);
};

} // namespace ar1
