#include "device/display.h"
#include "device/framebuffer.h"
#include <iomanip>
#include <iostream>
#include <sstream>

namespace ar1 {

Display::Display(int scale) : scale(scale) {}

Display::~Display() {
  if (texture)
    SDL_DestroyTexture(texture);
  if (renderer)
    SDL_DestroyRenderer(renderer);
  if (window)
    SDL_DestroyWindow(window);
  SDL_Quit();
}

bool Display::init() {
  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    std::cerr << "[Display] SDL Init failed: " << SDL_GetError() << std::endl;
    return false;
  }

  window =
      SDL_CreateWindow("AR1 VCPU by APRK", SDL_WINDOWPOS_CENTERED,
                       SDL_WINDOWPOS_CENTERED, width * scale, height * scale,
                       SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI);

  if (!window) {
    std::cerr << "[Display] Window creation failed: " << SDL_GetError()
              << std::endl;
    return false;
  }

  renderer = SDL_CreateRenderer(
      window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
  if (!renderer) {
    std::cerr << "[Display] Renderer creation failed: " << SDL_GetError()
              << std::endl;
    return false;
  }

  texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888,
                              SDL_TEXTUREACCESS_STREAMING, width, height);

  if (!texture) {
    std::cerr << "[Display] Texture creation failed: " << SDL_GetError()
              << std::endl;
    return false;
  }

  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
  SDL_RenderClear(renderer);
  SDL_RenderPresent(renderer);

  std::cout << "[Display] SDL2 initialized - " << width << "x" << height
            << " @ " << scale << "x" << std::endl;
  return true;
}

void Display::update(Framebuffer *fb) {
  if (!fb || !texture || !renderer)
    return;

  // Update texture with framebuffer data
  u8 *buffer = fb->get_buffer();

  void *pixels;
  int pitch;
  if (SDL_LockTexture(texture, nullptr, &pixels, &pitch) != 0) {
    return;
  }

  // Convert RGBA to SDL's format (RGBA8888 = ABGR in memory on little-endian)
  u32 *dst = (u32 *)pixels;
  for (u32 y = 0; y < (u32)height; y++) {
    for (u32 x = 0; x < (u32)width; x++) {
      u32 src_idx = (y * width + x) * 4;
      u8 r = buffer[src_idx + 0];
      u8 g = buffer[src_idx + 1];
      u8 b = buffer[src_idx + 2];
      u8 a = 255;
      // SDL_PIXELFORMAT_RGBA8888 expects RGBA in this order
      dst[y * (pitch / 4) + x] = (r << 24) | (g << 16) | (b << 8) | a;
    }
  }

  SDL_UnlockTexture(texture);

  // Render
  SDL_RenderClear(renderer);
  SDL_RenderCopy(renderer, texture, nullptr, nullptr);
  SDL_RenderPresent(renderer);

  frame_count++;
}

void Display::handle_events() {
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    switch (event.type) {
    case SDL_QUIT:
      quit_requested = true;
      break;
    case SDL_KEYDOWN: {
      char c = 0;
      SDL_Keycode key = event.key.keysym.sym;

      // Handle special keys
      if (key == SDLK_RETURN || key == SDLK_KP_ENTER) {
        c = '\n';
      } else if (key == SDLK_BACKSPACE) {
        c = '\b';
      } else if (key == SDLK_ESCAPE) {
        quit_requested = true;
      } else if (key >= SDLK_SPACE && key <= SDLK_z) {
        c = (char)key;
        // Handle shift for uppercase
        if (event.key.keysym.mod & KMOD_SHIFT) {
          if (c >= 'a' && c <= 'z') {
            c = c - 'a' + 'A';
          } else {
            // Number row shifts
            switch (c) {
            case '1':
              c = '!';
              break;
            case '2':
              c = '@';
              break;
            case '3':
              c = '#';
              break;
            case '4':
              c = '$';
              break;
            case '5':
              c = '%';
              break;
            case '6':
              c = '^';
              break;
            case '7':
              c = '&';
              break;
            case '8':
              c = '*';
              break;
            case '9':
              c = '(';
              break;
            case '0':
              c = ')';
              break;
            case '-':
              c = '_';
              break;
            case '=':
              c = '+';
              break;
            }
          }
        }
      } else if (key >= SDLK_KP_1 && key <= SDLK_KP_0) {
        // Numpad
        if (key == SDLK_KP_0)
          c = '0';
        else
          c = '1' + (key - SDLK_KP_1);
      } else if (key == SDLK_KP_PLUS)
        c = '+';
      else if (key == SDLK_KP_MINUS)
        c = '-';
      else if (key == SDLK_KP_MULTIPLY)
        c = '*';
      else if (key == SDLK_KP_DIVIDE)
        c = '/';

      if (c != 0) {
        std::lock_guard<std::mutex> lock(key_mutex);
        key_buffer += c;
      }
    } break;
    }
  }
}

char Display::get_key() {
  std::lock_guard<std::mutex> lock(key_mutex);
  if (key_buffer.empty())
    return 0;
  char c = key_buffer[0];
  key_buffer.erase(0, 1);
  return c;
}

void Display::set_title(const std::string &title) {
  if (window) {
    SDL_SetWindowTitle(window, title.c_str());
  }
}

void Display::show_stats(u64 instructions, double mips, u64 uptime_ms) {
  std::stringstream ss;
  ss << "AR1 VCPU | " << std::fixed << std::setprecision(2) << mips
     << " MIPS | " << instructions << " instr | " << (uptime_ms / 1000)
     << "s | FPS: " << frame_count;
  set_title(ss.str());
  frame_count = 0;
}

} // namespace ar1
