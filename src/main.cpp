#include "cpu/core.h"
#include "device/display.h"
#include "soc/bus.h"
#include "soc/loader.h"
#include <atomic>
#include <chrono>
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

const int NUM_CORES = 2; // Multi-core SMP!

// Global Control
std::atomic<bool> system_running(true);
std::atomic<bool> core_paused(false);

std::vector<std::unique_ptr<Core>> cores;
std::shared_ptr<Bus> g_bus;
std::unique_ptr<Display> g_display;
std::chrono::steady_clock::time_point start_time;

void print_banner() {
  std::cout << R"(
    _    ____  _  __     _______ _   _ 
   / \  |  _ \/ | \ \   / / ____| | | |
  / _ \ | |_) | |  \ \ / / |    | |_| |
 / ___ \|  _ <| |   \ V /| |    |  _  |
/_/   \_\_| \_\_|    \_/ |______|_| |_|
                                       
           AR1 Virtual CPU
)" << std::endl;
}

int main(int argc, char *argv[]) {
  print_banner();

  std::cout << "[AR1] ARM64 Virtual CPU - " << NUM_CORES << " Cores"
            << std::endl;
  std::cout << "[AR1] Press ESC to quit" << std::endl;
  std::cout << std::endl;

  if (argc < 2) {
    std::cerr << "Usage: ./ar1_vcpu <binary_image>" << std::endl;
    return 1;
  }
  std::string binary_path = argv[1];

  // Initialize SDL Display
  g_display = std::make_unique<ar1::Display>(3);
  if (!g_display->init()) {
    std::cerr << "[AR1] Failed to initialize display, running headless"
              << std::endl;
    g_display.reset();
  }

  g_bus = std::make_shared<ar1::Bus>();
  start_time = std::chrono::steady_clock::now();

  cores.resize(NUM_CORES);
  std::atomic<int> init_count(0);

  // Load binary first
  if (!Loader::load_binary(binary_path, RAM_BASE, g_bus.get())) {
    std::cerr << "[AR1] Load failed." << std::endl;
    return 1;
  }

  // Spawn CPU threads for each core
  std::vector<std::thread> cpu_threads;

  for (int i = 0; i < NUM_CORES; i++) {
    cpu_threads.emplace_back([i, &init_count]() {
      cores[i] = std::make_unique<Core>(g_bus, i);
      cores[i]->reset(RAM_BASE);

      std::cout << "[AR1] Core " << i << " started" << std::endl;
      init_count++;

      while (system_running) {
        if (!core_paused) {
          cores[i]->run(50000);
        } else {
          std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
      }

      std::cout << "[AR1] Core " << i << " stopped" << std::endl;
    });
  }

  // Wait for all cores to init
  while (init_count < NUM_CORES && system_running)
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

  std::cout << "[AR1] All " << NUM_CORES << " cores running!" << std::endl;

  // Main loop - Display & Input
  auto last_stats_time = std::chrono::steady_clock::now();

  while (system_running) {
    if (g_display) {
      g_display->handle_events();

      if (g_display->should_quit()) {
        system_running = false;
        break;
      }

      // Forward keyboard to UART
      while (g_display->has_key_event()) {
        char c = g_display->get_key();
        if (c != 0) {
          ar1::uart_push(c);
        }
      }

      // Update display with framebuffer
      g_display->update(g_bus->framebuffer.get(), cores[0].get(), g_bus.get());

      // Update stats every second
      auto now = std::chrono::steady_clock::now();
      auto stats_elapsed =
          std::chrono::duration_cast<std::chrono::milliseconds>(now -
                                                                last_stats_time)
              .count();
      if (stats_elapsed >= 1000) {
        u64 total_instr = 0;
        for (auto &c : cores) {
          if (c)
            total_instr += c->instructions_executed;
        }
        auto uptime = std::chrono::duration_cast<std::chrono::milliseconds>(
                          now - start_time)
                          .count();
        double mips = (double)total_instr / (uptime / 1000.0) / 1000000.0;

        std::stringstream ss;
        ss << "AR1 VCPU [" << NUM_CORES << " cores] | " << std::fixed
           << std::setprecision(2) << mips << " MIPS";
        g_display->set_title(ss.str());

        last_stats_time = now;
      }
    } else {
      std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(16)); // ~60 FPS
  }

  // Shutdown
  system_running = false;
  for (auto &t : cpu_threads) {
    if (t.joinable())
      t.join();
  }

  std::cout << "[AR1] Shutdown complete." << std::endl;
  return 0;
}
