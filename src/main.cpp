#include "cpu/core.h"
#include "soc/bus.h"
#include "soc/loader.h"
#include "soc/ram.h"
#include <atomic>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

using namespace ar1;

namespace ar1 {
extern void uart_push(char c);
}

const int NUM_CORES = 1;

// Global Control
std::atomic<bool> system_running(true);
std::atomic<bool> core_paused(true); // Start paused
std::vector<std::unique_ptr<Core>> cores;

void print_help() {
  std::cout << "Commands:\n"
            << "  run       - Resume execution\n"
            << "  stop      - Pause execution\n"
            << "  regs      - Dump registers (when paused)\n"
            << "  mem <addr> - Dump 16 bytes at hex addr\n"
            << "  exit      - Quit\n";
}

int main(int argc, char *argv[]) {
  std::cout << "===========================================" << std::endl;
  std::cout << "        AR1 INTERACTIVE SHELL             " << std::endl;
  std::cout << "===========================================" << std::endl;

  if (argc < 2) {
    std::cerr << "Usage: ./ar1_vcpu <binary_image>" << std::endl;
    return 1;
  }
  std::string binary_path = argv[1];

  auto bus = std::make_shared<ar1::Bus>();

  cores.resize(NUM_CORES);
  std::vector<std::thread> threads;
  std::atomic<int> init_count(0);

  for (int i = 0; i < NUM_CORES; i++) {
    threads.emplace_back([&, i]() {
      // Create Core
      cores[i] = std::make_unique<Core>(bus, i);
      init_count++;

      // Core 0 loads binary
      if (i == 0) {
        if (!Loader::load_binary(binary_path, RAM_BASE, bus.get())) {
          std::cerr << "Load failed." << std::endl;
          system_running = false;
        }
      }

      while (init_count < NUM_CORES)
        std::this_thread::yield(); // Barrier

      // Reset
      if (system_running)
        cores[i]->reset(RAM_BASE);

      // Loop
      while (system_running) {
        if (!core_paused) {
          cores[i]->run(10000);
        } else {
          std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
      }
    });
  }

  // Wait for init
  while (init_count < NUM_CORES && system_running)
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

  std::cout << "System Initialized. Cores Paused. type 'help' for commands."
            << std::endl;

  std::string line;
  while (system_running && std::getline(std::cin, line)) {
    std::stringstream ss(line);
    std::string cmd;
    ss >> cmd;

    if (cmd == "run") {
      core_paused = false;
      std::cout << "Resuming..." << std::endl;
    } else if (cmd == "stop") {
      core_paused = true;
      std::cout << "Paused." << std::endl;
    } else if (cmd == "regs" || cmd == "reg") {
      if (!core_paused)
        std::cout << "Pause first!" << std::endl;
      else {
        for (auto &c : cores)
          if (c)
            c->dump_regs();
      }
    } else if (cmd == "mem") {
      std::string addr_str;
      ss >> addr_str;
      try {
        u64 addr = std::stoull(addr_str, nullptr, 16);
        std::cout << "Mem [" << std::hex << addr << "]: ";
        for (int k = 0; k < 16; k++) {
          std::cout << std::setw(2) << std::setfill('0')
                    << (int)bus->ram->read8(addr + k - RAM_BASE) << " ";
        }
        std::cout << std::dec << std::endl;
      } catch (...) {
        std::cout << "Invalid Addr" << std::endl;
      }
    } else if (cmd == "type") {
      std::string input;
      std::getline(ss, input);
      // input contains leading space from 'type '
      for (char c : input) {
        if (c == ' ')
          continue; // Skip first space?
        ar1::uart_push(c);
      }
      ar1::uart_push('\n'); // Enter
      std::cout << "Sent." << std::endl;
    } else if (cmd == "exit") {
      system_running = false;
    } else if (cmd == "help") {
      print_help();
    }
  }

  for (auto &t : threads)
    t.join(); // or detach
  return 0;
}
