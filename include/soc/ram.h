#pragma once

#include "types.h"
#include <cstddef>
#include <vector>

namespace ar1 {

class RAM {
public:
  // Constructor allocates memory using mmap
  RAM(size_t size);
  ~RAM();

  // Raw pointer access for HVF mapping
  u8 *get_raw_ptr() const { return memory; }
  size_t get_size() const { return size; }

  // Read/Write
  u8 read8(size_t offset) const;
  u16 read16(size_t offset) const;
  u32 read32(size_t offset) const;
  u64 read64(size_t offset) const;

  void write8(size_t offset, u8 value);
  void write16(size_t offset, u16 value);
  void write32(size_t offset, u32 value);
  void write64(size_t offset, u64 value);

private:
  u8 *memory; // Raw pointer (mmap)
  size_t size;
};

} // namespace ar1
