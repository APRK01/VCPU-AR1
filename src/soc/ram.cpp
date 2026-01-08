#include "soc/ram.h"
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <sys/mman.h>
#include <unistd.h>

namespace ar1 {

#include <pthread.h>

// ...

RAM::RAM(size_t size) : size(size) {
  // Allocate Page-Aligned Memory using mmap (JIT for Exec)
  memory = (u8 *)mmap(NULL, size, PROT_READ | PROT_WRITE | PROT_EXEC,
                      MAP_ANON | MAP_PRIVATE | MAP_JIT, -1, 0);

  if (memory == MAP_FAILED) {
    std::cerr << "CRITICAL: mmap failed for RAM allocation!" << std::endl;
    throw std::runtime_error("RAM Allocation Failed");
  }

  // Enable Write Access for this thread (initially Exec-Only/RX)
  if (__builtin_available(macOS 11.0, *)) {
    pthread_jit_write_protect_np(0);
  }

  // Zero out memory
  memset(memory, 0, size);
}

RAM::~RAM() {
  if (memory && memory != MAP_FAILED) {
    munmap(memory, size);
  }
}

u8 RAM::read8(size_t offset) const {
  if (offset >= size)
    return 0;
  return memory[offset];
}

u16 RAM::read16(size_t offset) const {
  if (offset + 1 >= size)
    return 0;
  return *((u16 *)(&memory[offset]));
}

u32 RAM::read32(size_t offset) const {
  if (offset + 3 >= size)
    return 0;
  return *((u32 *)(&memory[offset]));
}

u64 RAM::read64(size_t offset) const {
  if (offset + 7 >= size)
    return 0;
  return *((u64 *)(&memory[offset]));
}

void RAM::write8(size_t offset, u8 value) {
  if (offset >= size)
    return;
  memory[offset] = value;
}

void RAM::write16(size_t offset, u16 value) {
  if (offset + 1 >= size)
    return;
  *((u16 *)(&memory[offset])) = value;
}

void RAM::write32(size_t offset, u32 value) {
  if (offset + 3 >= size)
    return;
  *((u32 *)(&memory[offset])) = value;
}

void RAM::write64(size_t offset, u64 value) {
  if (offset + 7 >= size)
    return;
  *((u64 *)(&memory[offset])) = value;
}

} // namespace ar1
