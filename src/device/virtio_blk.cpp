#include "device/virtio_blk.h"
#include "soc/bus.h" // For RAM constants
#include <cstring>
#include <iostream>
#include <pthread.h>

namespace ar1 {

// MMIO Offsets (Legacy/Modern mix)
#define VIRTIO_MMIO_MAGIC_VALUE 0x000
#define VIRTIO_MMIO_VERSION 0x004
#define VIRTIO_MMIO_DEVICE_ID 0x008
#define VIRTIO_MMIO_VENDOR_ID 0x00c
#define VIRTIO_MMIO_DEVICE_FEATURES 0x010
#define VIRTIO_MMIO_DEVICE_FEATURES_SEL 0x014
#define VIRTIO_MMIO_DRIVER_FEATURES 0x020
#define VIRTIO_MMIO_DRIVER_FEATURES_SEL 0x024
#define VIRTIO_MMIO_GUEST_PAGE_SIZE 0x028
#define VIRTIO_MMIO_QUEUE_SEL 0x030
#define VIRTIO_MMIO_QUEUE_NUM_MAX 0x034
#define VIRTIO_MMIO_QUEUE_NUM 0x038
#define VIRTIO_MMIO_QUEUE_ALIGN 0x03c
#define VIRTIO_MMIO_QUEUE_PFN 0x040
#define VIRTIO_MMIO_QUEUE_READY 0x044 // Modern
#define VIRTIO_MMIO_QUEUE_NOTIFY 0x050
#define VIRTIO_MMIO_INTERRUPT_STATUS 0x060
#define VIRTIO_MMIO_INTERRUPT_ACK 0x064
#define VIRTIO_MMIO_STATUS 0x070
#define VIRTIO_MMIO_QUEUE_DESC_LOW 0x080
#define VIRTIO_MMIO_QUEUE_DESC_HIGH 0x084
#define VIRTIO_MMIO_QUEUE_DRIVER_LOW 0x090
#define VIRTIO_MMIO_QUEUE_DRIVER_HIGH 0x094
#define VIRTIO_MMIO_QUEUE_DEVICE_LOW 0x0a0
#define VIRTIO_MMIO_QUEUE_DEVICE_HIGH 0x0a4

// VirtIO Descriptors
struct vring_desc {
  u64 addr;
  u32 len;
  u16 flags;
  u16 next;
};

// Flags
#define VRING_DESC_F_NEXT 1
#define VRING_DESC_F_WRITE 2

// Request Types
#define VIRTIO_BLK_T_IN 0
#define VIRTIO_BLK_T_OUT 1
#define VIRTIO_BLK_T_FLUSH 4

struct virtio_blk_req_header {
  u32 type;
  u32 reserved;
  u64 sector;
};

VirtIOBlock::VirtIOBlock(const std::string &image_path)
    : path(image_path), ram(nullptr), host_features(0), status(0),
      interrupt_status(0) {
  file.open(path, std::ios::in | std::ios::out | std::ios::binary);
  if (!file.is_open()) {
    std::cerr << "[VirtIO] Failed to open disk image: " << path
              << ". Trying fallback 'build/disk.img'." << std::endl;
    // Try fallback
    path = "build/disk.img";
    file.open(path, std::ios::in | std::ios::out | std::ios::binary);
  }
  if (!file.is_open()) {
    std::cerr << "[VirtIO] Failed to open disk image: " << path << std::endl;
    // Try creating it if not exists (handled by user cmd mostly)
  }

  // Init Queue
  queues[0].num = 128;
  queues[0].ready = false;
}

VirtIOBlock::~VirtIOBlock() {
  if (file.is_open())
    file.close();
}

u32 VirtIOBlock::read(u32 offset) {
  std::lock_guard<std::mutex> lock(mutex);
  if (offset == VIRTIO_MMIO_MAGIC_VALUE)
    return 0x74726976; // 'virt'
  if (offset == VIRTIO_MMIO_VERSION)
    return 2; // Modern
  if (offset == VIRTIO_MMIO_DEVICE_ID)
    return 2; // Block
  if (offset == VIRTIO_MMIO_VENDOR_ID)
    return 0x554D4551; // 'QEMU'

  if (offset == VIRTIO_MMIO_DEVICE_FEATURES) {
    if (host_features_sel == 0)
      return host_features;
    return 0;
  }

  if (offset == VIRTIO_MMIO_QUEUE_NUM_MAX)
    return 128;
  if (offset == VIRTIO_MMIO_QUEUE_READY)
    return queues[queue_sel].ready ? 1 : 0;
  if (offset == VIRTIO_MMIO_INTERRUPT_STATUS)
    return interrupt_status;
  if (offset == VIRTIO_MMIO_STATUS)
    return status;

  return 0;
}

void VirtIOBlock::write(u32 offset, u32 value) {
  std::lock_guard<std::mutex> lock(mutex);

  if (offset == VIRTIO_MMIO_DEVICE_FEATURES_SEL)
    host_features_sel = value;
  if (offset == VIRTIO_MMIO_DRIVER_FEATURES)
    guest_features = value;
  if (offset == VIRTIO_MMIO_DRIVER_FEATURES_SEL)
    guest_features_sel = value;

  if (offset == VIRTIO_MMIO_QUEUE_SEL)
    queue_sel = value;

  if (offset == VIRTIO_MMIO_QUEUE_NUM)
    queues[queue_sel].num = value;
  if (offset == VIRTIO_MMIO_QUEUE_DESC_LOW)
    queues[queue_sel].desc_lo = value;
  if (offset == VIRTIO_MMIO_QUEUE_DESC_HIGH)
    queues[queue_sel].desc_hi = value;
  if (offset == VIRTIO_MMIO_QUEUE_DRIVER_LOW)
    queues[queue_sel].avail_lo = value;
  if (offset == VIRTIO_MMIO_QUEUE_DRIVER_HIGH)
    queues[queue_sel].avail_hi = value;
  if (offset == VIRTIO_MMIO_QUEUE_DEVICE_LOW)
    queues[queue_sel].used_lo = value;
  if (offset == VIRTIO_MMIO_QUEUE_DEVICE_HIGH)
    queues[queue_sel].used_hi = value;

  if (offset == VIRTIO_MMIO_QUEUE_READY)
    queues[queue_sel].ready = (value == 1);

  if (offset == VIRTIO_MMIO_QUEUE_NOTIFY)
    notify_queue(value);

  if (offset == VIRTIO_MMIO_INTERRUPT_ACK)
    interrupt_status &= ~value;
  if (offset == VIRTIO_MMIO_STATUS)
    status = value;
}

void VirtIOBlock::notify_queue(u32 queue_idx) {
  if (queue_idx != 0 || !queues[0].ready || ram == nullptr)
    return;

  // Disable JIT write protection to allow host writes to guest RAM
  if (__builtin_available(macOS 11.0, *)) {
    pthread_jit_write_protect_np(0);
  }

  // Access Guest RAM
  // Avail Ring: Flags(2), Idx(2), Ring[QUEUE_NUM]
  u32 avail_addr = queues[0].avail_lo;
  u32 desc_addr = queues[0].desc_lo;
  u32 used_addr = queues[0].used_lo;

  // Sanity check addresses
  if (avail_addr < RAM_BASE || avail_addr >= RAM_BASE + RAM_SIZE)
    goto cleanup;
  if (desc_addr < RAM_BASE || desc_addr >= RAM_BASE + RAM_SIZE)
    goto cleanup;
  if (used_addr < RAM_BASE || used_addr >= RAM_BASE + RAM_SIZE)
    goto cleanup;

  {
    // Read idx from Avail Ring (Offset 2)
    u16 avail_idx = *(u16 *)(ram + (avail_addr - RAM_BASE) + 2);

    // Sanity check ring offset
    u32 ring_offset = avail_addr + 4 + ((avail_idx - 1) % queues[0].num) * 2;
    if (ring_offset < RAM_BASE || ring_offset >= RAM_BASE + RAM_SIZE)
      goto cleanup;

    u16 desc_head = *(u16 *)(ram + (ring_offset - RAM_BASE));

    process_request(desc_head);

    // Update Used Ring
    u16 *used_idx_ptr = (u16 *)(ram + (used_addr - RAM_BASE) + 2);
    u16 used_idx = *used_idx_ptr;

    u32 used_ring_entry = used_addr + 4 + (used_idx % queues[0].num) * 8;
    if (used_ring_entry < RAM_BASE ||
        used_ring_entry + 8 >= RAM_BASE + RAM_SIZE)
      goto cleanup;

    *(u32 *)(ram + (used_ring_entry - RAM_BASE)) = desc_head;
    *(u32 *)(ram + (used_ring_entry - RAM_BASE) + 4) = 0;

    *used_idx_ptr = used_idx + 1;
    interrupt_status |= 1;
  }

cleanup:
  // Re-enable JIT write protection
  if (__builtin_available(macOS 11.0, *)) {
    pthread_jit_write_protect_np(1);
  }
}

void VirtIOBlock::process_request(u32 desc_idx) {
  if (!file.is_open())
    return;

  // Read Descriptor 1 (Header)
  u32 desc_addr = queues[0].desc_lo + desc_idx * 16;
  if (desc_addr < RAM_BASE || desc_addr + 16 >= RAM_BASE + RAM_SIZE) {
    std::cerr << "Desc Addr OOB: " << std::hex << desc_addr << std::endl;
    return;
  }

  vring_desc desc;
  u64 desc_host_addr = (u64)(ram + (desc_addr - RAM_BASE));
  std::memcpy(&desc, (void *)desc_host_addr, sizeof(desc));

  if (desc.addr < RAM_BASE ||
      desc.addr + sizeof(virtio_blk_req_header) >= RAM_BASE + RAM_SIZE) {
    std::cerr << "Header Addr OOB: " << std::hex << desc.addr << std::endl;
    return;
  }

  virtio_blk_req_header header;
  u64 desc_header_addr = (u64)(ram + (desc.addr - RAM_BASE));
  std::memcpy(&header, (void *)desc_header_addr, sizeof(header));

  u32 type = header.type;
  u64 sector = header.sector;

  // Next Descriptor (Buffer)
  if (!(desc.flags & VRING_DESC_F_NEXT))
    return;
  u32 next_idx = desc.next;
  u32 next_desc_addr = queues[0].desc_lo + next_idx * 16;

  vring_desc desc2;
  u64 desc2_host_addr = (u64)(ram + (next_desc_addr - RAM_BASE));
  std::memcpy(&desc2, (void *)desc2_host_addr, sizeof(desc2));

  u64 buffer_pa = desc2.addr;
  u32 buffer_len = desc2.len;

  // Next Descriptor (Status)
  if (!(desc2.flags & VRING_DESC_F_NEXT))
    return;
  u32 status_idx = desc2.next;
  u32 status_desc_addr = queues[0].desc_lo + status_idx * 16;
  if (status_desc_addr < RAM_BASE ||
      status_desc_addr + 16 >= RAM_BASE + RAM_SIZE)
    return;

  vring_desc desc3;
  u64 desc3_host_addr = (u64)(ram + (status_desc_addr - RAM_BASE));
  std::memcpy(&desc3, (void *)desc3_host_addr, sizeof(desc3));

  if (desc3.addr < RAM_BASE || desc3.addr + 1 >= RAM_BASE + RAM_SIZE) {
    std::cerr << "Status Addr OOB" << std::endl;
    return;
  }

  u8 *status_ptr = (u8 *)(ram + (desc3.addr - RAM_BASE));

  if (type == VIRTIO_BLK_T_IN) {
    // Read from disk
    u64 offset = sector * 512;
    file.seekg(offset);
    file.read((char *)(ram + (buffer_pa - RAM_BASE)), buffer_len);
    *status_ptr = 0; // Success
  } else if (type == VIRTIO_BLK_T_OUT) {
    // Write to disk
    u64 offset = sector * 512;
    file.seekp(offset);
    file.write((char *)(ram + (buffer_pa - RAM_BASE)), buffer_len);
    *status_ptr = 0; // Success
  }
}

} // namespace ar1
