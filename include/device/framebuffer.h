#pragma once
#include "../types.h"
#include <mutex>

namespace ar1 {

// Simple framebuffer device - 320x200 @ 32bpp
#define FB_WIDTH 320
#define FB_HEIGHT 200
#define FB_BPP 4 // 32-bit RGBA

class Framebuffer {
public:
  Framebuffer();
  ~Framebuffer() = default;

  // MMIO registers
  u32 read(u32 offset);
  void write(u32 offset, u32 value);

  // Get framebuffer data for display
  u8 *get_buffer() { return buffer; }
  u32 get_width() const { return width; }
  u32 get_height() const { return height; }
  bool is_dirty() const { return dirty; }
  void clear_dirty() { dirty = false; }

  // Set RAM pointer for DMA access
  void set_ram(u8 *ram_ptr) { ram = ram_ptr; }

private:
  u8 buffer[FB_WIDTH * FB_HEIGHT * FB_BPP];
  u32 width = FB_WIDTH;
  u32 height = FB_HEIGHT;
  u32 pitch = FB_WIDTH * FB_BPP;
  bool dirty = false;
  bool enabled = false;
  u8 *ram = nullptr;
  std::mutex mutex;
};

} // namespace ar1
