#include "cpu/core.h"
#include "device/display.h"
#include "soc/bus.h"
#include "soc/loader.h"
#include <atomic>
#include <chrono>
#include <fstream>
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
std::atomic<bool> core_paused(false); // Start running immediately with SDL
std::vector<std::unique_ptr<Core>> cores;
std::shared_ptr<ar1::Bus> g_bus;
std::unique_ptr<ar1::Display> g_display;
std::chrono::steady_clock::time_point start_time;

void print_banner() {
  std::cout << "\n";
  std::cout << "  █████╗ ██████╗  ██╗    ██╗   ██╗ ██████╗██████╗ ██╗   ██╗\n";
  std::cout << " ██╔══██╗██╔══██╗███║    ██║   ██║██╔════╝██╔══██╗██║   ██║\n";
  std::cout << " ███████║██████╔╝╚██║    ██║   ██║██║     ██████╔╝██║   ██║\n";
  std::cout << " ██╔══██║██╔══██╗ ██║    ╚██╗ ██╔╝██║     ██╔═══╝ ██║   ██║\n";
  std::cout << " ██║  ██║██║  ██║ ██║     ╚████╔╝ ╚██████╗██║     ╚██████╔╝\n";
  std::cout << " ╚═╝  ╚═╝╚═╝  ╚═╝ ╚═╝      ╚═══╝   ╚═════╝╚═╝      ╚═════╝ \n";
  std::cout << "                        by APRK                           \n";
  std::cout << "\n";
}

int main(int argc, char *argv[]) {
  print_banner();

  std::cout << "[AR1] ARM64 Virtual CPU with SDL2 Graphics" << std::endl;
  std::cout << "[AR1] Press ESC to quit, keys go to VCPU" << std::endl;
  std::cout << std::endl;

  if (argc < 2) {
    std::cerr << "Usage: ./ar1_vcpu <binary_image>" << std::endl;
    return 1;
  }
  std::string binary_path = argv[1];

  // Initialize SDL Display
  g_display = std::make_unique<ar1::Display>(3); // 3x scale
  if (!g_display->init()) {
    std::cerr << "[AR1] Failed to initialize display, running headless"
              << std::endl;
    g_display.reset();
  }

  g_bus = std::make_shared<ar1::Bus>();
  start_time = std::chrono::steady_clock::now();

  cores.resize(NUM_CORES);
  std::atomic<int> init_count(0);

  // CPU Thread
  std::thread cpu_thread([&]() {
    cores[0] = std::make_unique<Core>(g_bus, 0);
    init_count++;

    if (!Loader::load_binary(binary_path, RAM_BASE, g_bus.get())) {
      std::cerr << "[AR1] Load failed." << std::endl;
      system_running = false;
      return;
    }

    cores[0]->reset(RAM_BASE);
    std::cout << "[AR1] CPU Running..." << std::endl;

    while (system_running) {
      if (!core_paused) {
        cores[0]->run(
            50000); // More instructions per batch for better performance
      } else {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
      }
    }
  });

  // Wait for CPU init
  while (init_count < NUM_CORES && system_running)
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

  // Main loop - Display & Input
  auto last_stats_time = std::chrono::steady_clock::now();

  while (system_running) {
    if (g_display) {
      // Handle SDL events (keyboard, quit)
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
      g_display->update(g_bus->framebuffer.get());

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
        g_display->show_stats(total_instr, mips, uptime);
        last_stats_time = now;
      }
    } else {
      // Headless mode - just sleep
      std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }

    // ~60 FPS
    std::this_thread::sleep_for(std::chrono::milliseconds(16));
  }

  std::cout << "\n[AR1] Shutting down..." << std::endl;

  // Final stats
  u64 total_instr = 0;
  for (auto &c : cores) {
    if (c)
      total_instr += c->instructions_executed;
  }
  auto uptime = std::chrono::duration_cast<std::chrono::seconds>(
                    std::chrono::steady_clock::now() - start_time)
                    .count();

  std::cout << "[AR1] Total instructions: " << total_instr << std::endl;
  if (uptime > 0) {
    std::cout << "[AR1] Average MIPS: " << std::fixed << std::setprecision(2)
              << (double)total_instr / uptime / 1000000.0 << std::endl;
  }

  cpu_thread.join();

  std::cout << "[AR1] Goodbye!" << std::endl;
  return 0;
}
