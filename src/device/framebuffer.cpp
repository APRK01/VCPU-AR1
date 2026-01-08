#include "device/framebuffer.h"
#include "soc/bus.h"
#include <cstring>
#include <iostream>

namespace ar1 {

// MMIO Register Offsets
#define FB_REG_WIDTH 0x00
#define FB_REG_HEIGHT 0x04
#define FB_REG_PITCH 0x08
#define FB_REG_ENABLED 0x0C
#define FB_REG_FLUSH 0x10
#define FB_REG_PIXEL 0x14 // Write pixel: [31:16]=x, [15:0]=y, then write color
#define FB_REG_COLOR 0x18 // Color to write
#define FB_REG_FILL 0x1C  // Fill entire screen with color

static u32 pending_x = 0;
static u32 pending_y = 0;

Framebuffer::Framebuffer() {
  std::memset(buffer, 0, sizeof(buffer)); // Black screen
}

u32 Framebuffer::read(u32 offset) {
  std::lock_guard<std::mutex> lock(mutex);

  switch (offset) {
  case FB_REG_WIDTH:
    return width;
  case FB_REG_HEIGHT:
    return height;
  case FB_REG_PITCH:
    return pitch;
  case FB_REG_ENABLED:
    return enabled ? 1 : 0;
  default:
    return 0;
  }
}

void Framebuffer::write(u32 offset, u32 value) {
  std::lock_guard<std::mutex> lock(mutex);

  switch (offset) {
  case FB_REG_ENABLED:
    enabled = (value != 0);
    break;

  case FB_REG_FLUSH:
    dirty = true;
    break;

  case FB_REG_PIXEL:
    // Format: [31:16] = x, [15:0] = y
    pending_x = (value >> 16) & 0xFFFF;
    pending_y = value & 0xFFFF;
    break;

  case FB_REG_COLOR:
    // Write color at pending_x, pending_y
    if (pending_x < width && pending_y < height) {
      u32 idx = (pending_y * width + pending_x) * FB_BPP;
      buffer[idx + 0] = (value >> 0) & 0xFF;  // R
      buffer[idx + 1] = (value >> 8) & 0xFF;  // G
      buffer[idx + 2] = (value >> 16) & 0xFF; // B
      buffer[idx + 3] = (value >> 24) & 0xFF; // A
      dirty = true;
    }
    break;

  case FB_REG_FILL:
    // Fill entire screen with color
    for (u32 i = 0; i < width * height; i++) {
      buffer[i * FB_BPP + 0] = (value >> 0) & 0xFF;  // R
      buffer[i * FB_BPP + 1] = (value >> 8) & 0xFF;  // G
      buffer[i * FB_BPP + 2] = (value >> 16) & 0xFF; // B
      buffer[i * FB_BPP + 3] = 0xFF;                 // A
    }
    dirty = true;
    break;
  }
}

} // namespace ar1
