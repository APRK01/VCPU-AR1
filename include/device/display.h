#pragma once
#include "../types.h"
#include <SDL2/SDL.h>
#include <atomic>
#include <mutex>
#include <string>

namespace ar1 {

class Framebuffer;

class Display {
public:
  Display(int scale = 3);
  ~Display();

  bool init();
  void update(Framebuffer *fb);
  void handle_events();
  void show_stats(u64 instructions, double mips, u64 uptime_ms);

  bool should_quit() const { return quit_requested; }
  bool has_key_event() const { return !key_buffer.empty(); }
  char get_key();

  void set_title(const std::string &title);

private:
  SDL_Window *window = nullptr;
  SDL_Renderer *renderer = nullptr;
  SDL_Texture *texture = nullptr;

  int scale;
  int width = 320;
  int height = 200;

  std::atomic<bool> quit_requested{false};
  std::string key_buffer;
  std::mutex key_mutex;

  u64 last_instructions = 0;
  u64 frame_count = 0;
};

} // namespace ar1
