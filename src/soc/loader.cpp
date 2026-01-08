#include "soc/loader.h"
#include <cstring>
#include <fstream>
#include <iostream>
#include <libkern/OSCacheControl.h>
#include <sys/stat.h>
#include <vector>

namespace ar1 {

bool Loader::load_binary(const std::string &path, u64 addr, Bus *bus) {
  std::ifstream file(path, std::ios::binary | std::ios::ate);
  if (!file.is_open())
    return false;

  std::streamsize size = file.tellg();
  file.seekg(0, std::ios::beg);

  std::vector<char> buffer(static_cast<size_t>(size));
  if (!file.read(buffer.data(), size))
    return false;

  if (addr >= RAM_BASE && (addr + (u64)size) <= (RAM_BASE + RAM_SIZE)) {
    u8 *ram_ptr = bus->ram->get_raw_ptr();
    void *dest = ram_ptr + (addr - RAM_BASE);
#include <pthread.h>

    if (__builtin_available(macOS 11.0, *)) {
      pthread_jit_write_protect_np(0); // Enable Write
    }

    std::memcpy(dest, buffer.data(), (size_t)size);

    if (__builtin_available(macOS 11.0, *)) {
      pthread_jit_write_protect_np(1); // Enable Exec (Restore)
    }

    // Debug: Print first 4 bytes
    u32 *code = (u32 *)dest;
    std::cout << "[Loader] Loaded code at " << std::hex << addr << ": "
              << std::hex << code[0] << " " << code[1] << std::dec << std::endl;

    sys_icache_invalidate(dest, (size_t)size);
  } else {
    for (size_t i = 0; i < (size_t)size; i++) {
      bus->write8(addr + i, (u8)buffer[i]);
    }
  }

  return true;
}

} // namespace ar1
