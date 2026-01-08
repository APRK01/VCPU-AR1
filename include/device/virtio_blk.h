#pragma once
#include "../types.h"
#include <fstream>
#include <mutex>
#include <vector>

namespace ar1 {

// 4K Page Size
#define VIRTIO_PAGE_SIZE 4096

// Simplified Queue
struct VirtQueue {
  u32 num;     // Queue size (must be power of 2)
  u32 desc_lo; // Address of descriptors
  u32 desc_hi;
  u32 avail_lo; // Address of available ring
  u32 avail_hi;
  u32 used_lo; // Address of used ring
  u32 used_hi;
  bool ready;
};

class VirtIOBlock {
public:
  VirtIOBlock(const std::string &image_path);
  ~VirtIOBlock();

  u32 read(u32 offset);
  void write(u32 offset, u32 value);

  // Helper to signal interrupt to GIC
  bool is_irq_pending() const { return interrupt_status != 0; }
  void clear_irq(u32 val) { interrupt_status &= ~val; }

  // Needs access to RAM to read descriptors/buffers
  void set_ram(u8 *ram_ptr) { this->ram = ram_ptr; }

private:
  std::string path;
  std::fstream file;
  std::mutex mutex;
  u8 *ram; // Raw pointer to system RAM for DMA

  // Registers
  u32 host_features; // Device features
  u32 guest_features;
  u32 host_features_sel;
  u32 guest_features_sel;
  u32 guest_page_size;
  u32 queue_sel;
  u32 queue_num;
  u32 status;
  u32 interrupt_status;

  VirtQueue queues[1]; // Only 1 queue for now

  void notify_queue(u32 queue_idx);
  void process_request(u32 desc_idx);
};

} // namespace ar1
