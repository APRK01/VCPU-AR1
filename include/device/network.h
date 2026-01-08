#pragma once
#include "../types.h"
#include <atomic>
#include <cstring>
#include <mutex>
#include <string>
#include <thread>

namespace ar1 {

// Network Device Memory Map
// 0x0D000000 - NET_STATUS (read: 0=idle, 1=busy, 2=done, 3=error)
// 0x0D000004 - NET_CMD (write: 1=ping, 2=fetch)
// 0x0D000008 - NET_DATA_LEN (read: response length)
// 0x0D00000C - NET_TX_CHAR (write: add char to TX buffer)
// 0x0D000010 - NET_RX_CHAR (read: get next char from RX buffer)
// 0x0D000014 - NET_CLEAR (write: clear buffers)

constexpr u64 NET_BASE = 0x0D000000;

enum NetStatus : u32 {
  NET_IDLE = 0,
  NET_BUSY = 1,
  NET_DONE = 2,
  NET_ERROR = 3
};

enum NetCmd : u32 { NET_CMD_NONE = 0, NET_CMD_PING = 1, NET_CMD_FETCH = 2 };

class Network {
public:
  Network() = default;
  ~Network() {
    if (worker_thread.joinable()) {
      worker_thread.join();
    }
  }

  u32 read(u32 offset) {
    std::lock_guard<std::mutex> lock(mutex);
    switch (offset) {
    case 0x00:
      return status;
    case 0x08:
      return (u32)rx_buffer.size();
    case 0x10: {
      if (rx_index < rx_buffer.size()) {
        return (u32)(u8)rx_buffer[rx_index++];
      }
      return 0;
    }
    default:
      return 0;
    }
  }

  void write(u32 offset, u32 value) {
    std::lock_guard<std::mutex> lock(mutex);
    switch (offset) {
    case 0x04: // CMD
      if (value == NET_CMD_PING) {
        start_ping();
      } else if (value == NET_CMD_FETCH) {
        start_fetch();
      }
      break;
    case 0x0C: // TX_CHAR
      tx_buffer += (char)value;
      break;
    case 0x14: // CLEAR
      tx_buffer.clear();
      rx_buffer.clear();
      rx_index = 0;
      status = NET_IDLE;
      break;
    }
  }

private:
  void start_ping() {
    if (status == NET_BUSY)
      return;
    status = NET_BUSY;
    std::string host = tx_buffer;
    tx_buffer.clear();

    worker_thread = std::thread([this, host]() {
      // Simple ping using system command
      std::string cmd = "ping -c 1 -t 2 " + host + " 2>&1";
      FILE *pipe = popen(cmd.c_str(), "r");
      std::string result;
      if (pipe) {
        char buffer[256];
        while (fgets(buffer, sizeof(buffer), pipe)) {
          result += buffer;
        }
        int ret = pclose(pipe);

        std::lock_guard<std::mutex> lock(mutex);
        rx_buffer = result;
        rx_index = 0;
        status = (ret == 0) ? NET_DONE : NET_ERROR;
      } else {
        std::lock_guard<std::mutex> lock(mutex);
        rx_buffer = "Failed to execute ping";
        rx_index = 0;
        status = NET_ERROR;
      }
    });
    worker_thread.detach();
  }

  void start_fetch() {
    if (status == NET_BUSY)
      return;
    status = NET_BUSY;
    std::string url = tx_buffer;
    tx_buffer.clear();

    worker_thread = std::thread([this, url]() {
      // Simple HTTP fetch using curl
      std::string cmd = "curl -s --max-time 5 \"" + url + "\" 2>&1";
      FILE *pipe = popen(cmd.c_str(), "r");
      std::string result;
      if (pipe) {
        char buffer[1024];
        while (fgets(buffer, sizeof(buffer), pipe)) {
          result += buffer;
          if (result.size() > 4096)
            break; // Limit response
        }
        int ret = pclose(pipe);

        std::lock_guard<std::mutex> lock(mutex);
        rx_buffer = result;
        rx_index = 0;
        status = (ret == 0) ? NET_DONE : NET_ERROR;
      } else {
        std::lock_guard<std::mutex> lock(mutex);
        rx_buffer = "Failed to execute curl";
        rx_index = 0;
        status = NET_ERROR;
      }
    });
    worker_thread.detach();
  }

  std::mutex mutex;
  std::atomic<u32> status{NET_IDLE};
  std::string tx_buffer;
  std::string rx_buffer;
  size_t rx_index = 0;
  std::thread worker_thread;
};

} // namespace ar1
