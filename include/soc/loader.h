#pragma once
#include "../types.h"
#include "bus.h"
#include <string>

namespace ar1 {

class Loader {
public:
  static bool load_binary(const std::string &path, u64 addr, Bus *bus);
};

} // namespace ar1
