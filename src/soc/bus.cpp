#include "soc/bus.h"

namespace ar1 {

Bus::Bus() {
  ram = std::make_shared<RAM>(RAM_SIZE);
  vram = std::make_shared<RAM>(VRAM_SIZE);
  uart = std::make_shared<UART>();
  gic = std::make_shared<GIC>();
  virtio_blk = std::make_shared<VirtIOBlock>("disk.img");
  virtio_blk->set_ram(ram->get_raw_ptr());
  framebuffer = std::make_shared<Framebuffer>();
  framebuffer->set_ram(ram->get_raw_ptr());
}

u8 Bus::read8(u64 addr) {
  if (addr >= RAM_BASE && addr < RAM_BASE + RAM_SIZE) {
    return ram->read8(addr - RAM_BASE);
  }
  if (addr >= VRAM_BASE && addr < VRAM_BASE + VRAM_SIZE) {
    return vram->read8(addr - VRAM_BASE);
  }
  return 0;
}

u16 Bus::read16(u64 addr) {
  if (addr >= RAM_BASE && addr < RAM_BASE + RAM_SIZE) {
    return ram->read16(addr - RAM_BASE);
  }
  if (addr >= VRAM_BASE && addr < VRAM_BASE + VRAM_SIZE) {
    return vram->read16(addr - VRAM_BASE);
  }
  return 0;
}

u32 Bus::read32(u64 addr) {
  if (addr >= RAM_BASE && addr < RAM_BASE + RAM_SIZE) {
    return ram->read32(addr - RAM_BASE);
  }
  if (addr >= VRAM_BASE && addr < VRAM_BASE + VRAM_SIZE) {
    return vram->read32(addr - VRAM_BASE);
  }
  if (addr >= GIC_DIST_BASE && addr < GIC_DIST_BASE + 0x1000) {
    return gic->read_dist(addr - GIC_DIST_BASE);
  }
  if (addr >= GIC_CPU_BASE && addr < GIC_CPU_BASE + 0x2000) {
    return gic->read_cpu(addr - GIC_CPU_BASE);
  }
  if (addr >= VIRTIO_BASE && addr < VIRTIO_BASE + 0x200) {
    return virtio_blk->read(addr - VIRTIO_BASE);
  }
  if (addr >= FB_BASE && addr < FB_BASE + 0x100) {
    return framebuffer->read(addr - FB_BASE);
  }
  return 0;
}

u64 Bus::read64(u64 addr) {
  if (addr >= RAM_BASE && addr < RAM_BASE + RAM_SIZE) {
    return ram->read64(addr - RAM_BASE);
  }
  if (addr >= VRAM_BASE && addr < VRAM_BASE + VRAM_SIZE) {
    return vram->read64(addr - VRAM_BASE);
  }
  return 0;
}

void Bus::write8(u64 addr, u8 value) {
  if (addr >= RAM_BASE && addr < RAM_BASE + RAM_SIZE) {
    ram->write8(addr - RAM_BASE, value);
  } else if (addr >= VRAM_BASE && addr < VRAM_BASE + VRAM_SIZE) {
    vram->write8(addr - VRAM_BASE, value);
  }
}

void Bus::write16(u64 addr, u16 value) {
  if (addr >= RAM_BASE && addr < RAM_BASE + RAM_SIZE) {
    ram->write16(addr - RAM_BASE, value);
  } else if (addr >= VRAM_BASE && addr < VRAM_BASE + VRAM_SIZE) {
    vram->write16(addr - VRAM_BASE, value);
  }
}

void Bus::write32(u64 addr, u32 value) {
  if (addr >= RAM_BASE && addr < RAM_BASE + RAM_SIZE) {
    ram->write32(addr - RAM_BASE, value);
  } else if (addr >= VRAM_BASE && addr < VRAM_BASE + VRAM_SIZE) {
    vram->write32(addr - VRAM_BASE, value);
  } else if (addr >= GIC_DIST_BASE && addr < GIC_DIST_BASE + 0x1000) {
    gic->write_dist(addr - GIC_DIST_BASE, value);
  } else if (addr >= GIC_CPU_BASE && addr < GIC_CPU_BASE + 0x2000) {
    gic->write_cpu(addr - GIC_CPU_BASE, value);
  } else if (addr >= VIRTIO_BASE && addr < VIRTIO_BASE + 0x200) {
    virtio_blk->write(addr - VIRTIO_BASE, value);
  } else if (addr >= FB_BASE && addr < FB_BASE + 0x100) {
    framebuffer->write(addr - FB_BASE, value);
  }
}

void Bus::write64(u64 addr, u64 value) {
  if (addr >= RAM_BASE && addr < RAM_BASE + RAM_SIZE) {
    ram->write64(addr - RAM_BASE, value);
  } else if (addr >= VRAM_BASE && addr < VRAM_BASE + VRAM_SIZE) {
    vram->write64(addr - VRAM_BASE, value);
  }
}

} // namespace ar1
