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
std::shared_ptr<ar1::Bus> g_bus;
std::chrono::steady_clock::time_point start_time;

void print_help() {
  std::cout << "Commands:\n"
            << "  run       - Resume execution\n"
            << "  stop      - Pause execution\n"
            << "  regs      - Dump registers (when paused)\n"
            << "  mem <addr>- Dump 16 bytes at hex addr\n"
            << "  perf      - Show performance stats\n"
            << "  fb        - Save framebuffer to fb_output.ppm\n"
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

  g_bus = std::make_shared<ar1::Bus>();
  start_time = std::chrono::steady_clock::now();

  cores.resize(NUM_CORES);
  std::vector<std::thread> threads;
  std::atomic<int> init_count(0);

  for (int i = 0; i < NUM_CORES; i++) {
    threads.emplace_back([&, i]() {
      // Create Core
      cores[i] = std::make_unique<Core>(g_bus, i);
      init_count++;

      // Core 0 loads binary
      if (i == 0) {
        if (!Loader::load_binary(binary_path, RAM_BASE, g_bus.get())) {
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
                    << (int)g_bus->ram->read8(addr + k - RAM_BASE) << " ";
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
    } else if (cmd == "perf") {
      // Performance stats
      auto now = std::chrono::steady_clock::now();
      auto elapsed =
          std::chrono::duration_cast<std::chrono::seconds>(now - start_time)
              .count();
      u64 total_instr = 0;
      for (auto &c : cores) {
        if (c)
          total_instr += c->instructions_executed;
      }
      std::cout << "\n=== Performance Stats ===" << std::endl;
      std::cout << "Uptime: " << elapsed << " seconds" << std::endl;
      std::cout << "Instructions: " << total_instr << std::endl;
      if (elapsed > 0) {
        std::cout << "IPS: " << (total_instr / elapsed) << std::endl;
        std::cout << "MIPS: " << std::fixed << std::setprecision(2)
                  << (double)total_instr / elapsed / 1000000.0 << std::endl;
      }
      std::cout << "=========================" << std::endl;
    } else if (cmd == "fb") {
      // Save framebuffer to PPM
      auto fb = g_bus->framebuffer;
      std::ofstream file("fb_output.ppm", std::ios::binary);
      if (file) {
        file << "P6\n"
             << fb->get_width() << " " << fb->get_height() << "\n255\n";
        u8 *buf = fb->get_buffer();
        for (u32 y = 0; y < fb->get_height(); y++) {
          for (u32 x = 0; x < fb->get_width(); x++) {
            u32 idx = (y * fb->get_width() + x) * 4;
            file.put(buf[idx + 0]); // R
            file.put(buf[idx + 1]); // G
            file.put(buf[idx + 2]); // B
          }
        }
        file.close();
        std::cout << "Saved framebuffer to fb_output.ppm" << std::endl;
      } else {
        std::cout << "Failed to save framebuffer" << std::endl;
      }
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
