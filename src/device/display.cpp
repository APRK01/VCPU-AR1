#include "device/display.h"
#include "cpu/core.h"
#include "device/framebuffer.h"
#include "soc/bus.h"
#include "soc/ram.h"
#include <iomanip>
#include <iostream>
#include <sstream>
#include <vector>

#include "backends/imgui_impl_sdl2.h"
#include "backends/imgui_impl_sdlrenderer2.h"
#include "imgui.h"

namespace ar1 {

Display::Display(int scale) : scale(scale) {}

Display::~Display() {
  if (capstone_ready)
    cs_close(&capstone_handle);

  ImGui_ImplSDLRenderer2_Shutdown();
  ImGui_ImplSDL2_Shutdown();
  ImGui::DestroyContext();

  if (texture)
    SDL_DestroyTexture(texture);
  if (renderer)
    SDL_DestroyRenderer(renderer);
  if (window)
    SDL_DestroyWindow(window);
  SDL_Quit();
}

void Display::apply_theme() {
  ImGuiStyle &style = ImGui::GetStyle();

  // Apple-like Minimalist Theme
  style.WindowRounding = 6.0f;
  style.FrameRounding = 4.0f;
  style.PopupRounding = 4.0f;
  style.ScrollbarRounding = 4.0f;
  style.GrabRounding = 4.0f;
  style.TabRounding = 4.0f;

  style.WindowBorderSize = 0.0f;
  style.FrameBorderSize = 0.0f;

  // Colors (Light Theme)
  ImVec4 *colors = style.Colors;
  colors[ImGuiCol_Text] = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
  colors[ImGuiCol_TextDisabled] = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
  colors[ImGuiCol_WindowBg] =
      ImVec4(0.96f, 0.96f, 0.96f, 0.95f); // Frosted/Light
  colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
  colors[ImGuiCol_PopupBg] = ImVec4(1.00f, 1.00f, 1.00f, 0.98f);
  colors[ImGuiCol_Border] = ImVec4(0.00f, 0.00f, 0.00f, 0.15f);
  colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
  colors[ImGuiCol_FrameBg] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
  colors[ImGuiCol_FrameBgHovered] = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
  colors[ImGuiCol_FrameBgActive] = ImVec4(0.85f, 0.85f, 0.85f, 1.00f);
  colors[ImGuiCol_TitleBg] = ImVec4(0.92f, 0.92f, 0.92f, 1.00f);
  colors[ImGuiCol_TitleBgActive] = ImVec4(0.92f, 0.92f, 0.92f, 1.00f);
  colors[ImGuiCol_TitleBgCollapsed] = ImVec4(1.00f, 1.00f, 1.00f, 0.51f);
  colors[ImGuiCol_MenuBarBg] = ImVec4(0.95f, 0.95f, 0.95f, 1.00f);
  colors[ImGuiCol_ScrollbarBg] = ImVec4(0.98f, 0.98f, 0.98f, 0.53f);
  colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.69f, 0.69f, 0.69f, 1.00f);
  colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.49f, 0.49f, 0.49f, 1.00f);
  colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.49f, 0.49f, 0.49f, 1.00f);
  colors[ImGuiCol_CheckMark] = ImVec4(0.20f, 0.50f, 1.00f, 1.00f); // Apple Blue
  colors[ImGuiCol_SliderGrab] = ImVec4(0.20f, 0.50f, 1.00f, 1.00f);
  colors[ImGuiCol_SliderGrabActive] = ImVec4(0.20f, 0.50f, 1.00f, 1.00f);
  colors[ImGuiCol_Button] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
  colors[ImGuiCol_ButtonHovered] = ImVec4(0.95f, 0.95f, 0.95f, 1.00f);
  colors[ImGuiCol_ButtonActive] = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
  colors[ImGuiCol_Header] = ImVec4(0.20f, 0.50f, 1.00f, 0.15f);
  colors[ImGuiCol_HeaderHovered] = ImVec4(0.20f, 0.50f, 1.00f, 0.25f);
  colors[ImGuiCol_HeaderActive] = ImVec4(0.20f, 0.50f, 1.00f, 0.35f);
  colors[ImGuiCol_Separator] = ImVec4(0.00f, 0.00f, 0.00f, 0.10f);
}

bool Display::init() {
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0) {
    std::cerr << "[Display] SDL Init failed: " << SDL_GetError() << std::endl;
    return false;
  }

  // Initialize Capstone
  if (cs_open(CS_ARCH_ARM64, CS_MODE_ARM, &capstone_handle) == CS_ERR_OK) {
    capstone_ready = true;
    // cs_option(capstone_handle, CS_OPT_DETAIL, CS_OPT_ON); // Debug details
  } else {
    std::cerr << "[Display] Failed to scale capstone disassembler" << std::endl;
  }

  // SDL Window flags
  SDL_WindowFlags window_flags =
      (SDL_WindowFlags)(SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI);

  window = SDL_CreateWindow("AR1 VCPU - System Monitor", SDL_WINDOWPOS_CENTERED,
                            SDL_WINDOWPOS_CENTERED,
                            width * scale + 400, // Extra width for debug panel
                            height * scale, window_flags);

  if (!window)
    return false;

  renderer = SDL_CreateRenderer(
      window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
  if (!renderer)
    return false;

  // Setup ImGui
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  (void)io;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

  // HighDPI Scaling
  io.FontGlobalScale = 2.0f;             // Scale text by 200% for Retina
  ImGuiStyle &style = ImGui::GetStyle(); // Get style before applying theme
  style.ScaleAllSizes(2.0f);             // Scale UI elements by 200%

  apply_theme();

  ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
  ImGui_ImplSDLRenderer2_Init(renderer);

  texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888,
                              SDL_TEXTUREACCESS_STREAMING, width, height);

  return (texture != nullptr);
}

void Display::draw_debug_overlay(Core *core, Bus *bus) {
  // Disassembly Window
  ImGui::SetNextWindowPos(ImVec2(width * scale + 20, 20),
                          ImGuiCond_FirstUseEver); // Add padding
  ImGui::SetNextWindowSize(ImVec2(450, height * scale - 40),
                           ImGuiCond_FirstUseEver); // Wider

  ImGui::Begin("System Monitor", nullptr, ImGuiWindowFlags_NoCollapse);

  ImGui::TextDisabled("VCPU State: RUNNING");
  ImGui::Separator();

  if (ImGui::CollapsingHeader("Registers", ImGuiTreeNodeFlags_DefaultOpen)) {
    if (ImGui::BeginTable("regs", 2, ImGuiTableFlags_RowBg)) {
      for (int i = 0; i < 31; i++) {
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::TextColored(ImVec4(0.4f, 0.4f, 0.4f, 1.0f), "X%d", i);
        ImGui::TableSetColumnIndex(1);
        ImGui::Text("0x%016llX", core->regs.x[i]);
      }
      ImGui::TableNextRow();
      ImGui::TableSetColumnIndex(0);
      ImGui::TextColored(ImVec4(0.8f, 0.2f, 0.2f, 1.0f), "PC");
      ImGui::TableSetColumnIndex(1);
      ImGui::Text("0x%016llX", core->regs.pc);

      ImGui::EndTable();
    }
  }

  ImGui::Separator();

  if (ImGui::CollapsingHeader("Disassembly", ImGuiTreeNodeFlags_DefaultOpen)) {
    u64 pc = core->get_pc();
    // Read memory around PC
    const int INSTR_COUNT = 8;
    u8 code[INSTR_COUNT * 4];
    u64 base_pc = pc;

    // Read instructions from bus
    for (int i = 0; i < INSTR_COUNT * 4; i++) {
      // Safe read byte-by-byte for now (unoptimized)
      code[i] = bus->ram->read8(base_pc + i - RAM_BASE);
    }

    if (capstone_ready) {
      cs_insn *insn;
      size_t count =
          cs_disasm(capstone_handle, code, sizeof(code), base_pc, 0, &insn);
      if (count > 0) {
        for (size_t j = 0; j < count; j++) {
          bool is_current = (insn[j].address == pc);
          if (is_current) {
            ImGui::TextColored(ImVec4(0.0f, 0.5f, 1.0f, 1.0f),
                               "-> %08llX:  %s  %s", insn[j].address,
                               insn[j].mnemonic, insn[j].op_str);
          } else {
            ImGui::Text("%08llX:  %s  %s", insn[j].address, insn[j].mnemonic,
                        insn[j].op_str);
          }
        }
        cs_free(insn, count);
      } else {
        ImGui::Text("Failed to disassemble");
      }
    }
  }

  ImGui::End();
}

void Display::update(Framebuffer *fb, Core *core, Bus *bus) {
  // Capture mouse state and forward to VCPU
  int mx, my;
  u32 buttons = SDL_GetMouseState(&mx, &my);

  // Check if mouse is in VCPU screen area (not ImGui panel)
  if (mx >= 20 && mx < 20 + width * scale && my >= 20 &&
      my < 20 + height * scale) {
    // Translate to VCPU coordinates (0-319, 0-199)
    i32 vcpu_x = (mx - 20) / scale;
    i32 vcpu_y = (my - 20) / scale;

    bus->mouse->set_position(vcpu_x, vcpu_y);
    bus->mouse->set_present(true);

    u32 button_state = 0;
    if (buttons & SDL_BUTTON(SDL_BUTTON_LEFT))
      button_state |= 1;
    if (buttons & SDL_BUTTON(SDL_BUTTON_RIGHT))
      button_state |= 2;
    if (buttons & SDL_BUTTON(SDL_BUTTON_MIDDLE))
      button_state |= 4;
    bus->mouse->set_buttons(button_state);
  } else {
    bus->mouse->set_present(false);
  }

  // Start ImGui frame
  ImGui_ImplSDLRenderer2_NewFrame();
  ImGui_ImplSDL2_NewFrame();
  ImGui::NewFrame();

  // Draw our custom UI
  draw_debug_overlay(core, bus);

  // Rendering
  ImGui::Render();

  if (!fb || !texture || !renderer)
    return;

  // --- Draw VCPU Framebuffer portion ---
  u8 *buffer = fb->get_buffer();
  void *pixels;
  int pitch;
  if (SDL_LockTexture(texture, nullptr, &pixels, &pitch) == 0) {
    u32 *dst = (u32 *)pixels;
    for (u32 y = 0; y < (u32)height; y++) {
      for (u32 x = 0; x < (u32)width; x++) {
        u32 src_idx = (y * width + x) * 4;
        u8 r = buffer[src_idx + 0];
        u8 g = buffer[src_idx + 1];
        u8 b = buffer[src_idx + 2];
        dst[y * (pitch / 4) + x] = (r << 24) | (g << 16) | (b << 8) | 255;
      }
    }
    SDL_UnlockTexture(texture);
  }

  // Clear background (Darker "Desktop" Grey)
  SDL_SetRenderDrawColor(renderer, 50, 50, 55, 255); // Dark aesthetic
  SDL_RenderClear(renderer);

  // 1. Draw VCPU Screen with a border
  SDL_Rect dest_rect = {20, 20, width * scale, height * scale}; // Add padding

  // Draw white border for VCPU screen
  SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
  SDL_Rect border_rect = {18, 18, width * scale + 4, height * scale + 4};
  SDL_RenderFillRect(renderer, &border_rect);

  SDL_RenderCopy(renderer, texture, nullptr, &dest_rect);

  // 2. Draw ImGui Overlay
  ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), renderer);

  SDL_RenderPresent(renderer);
  frame_count++;
}

void Display::handle_events() {
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    ImGui_ImplSDL2_ProcessEvent(&event); // Send to ImGui

    switch (event.type) {
    case SDL_QUIT:
      quit_requested = true;
      break;
    case SDL_KEYDOWN:
      // Only capture keyboard if ImGui doesn't want it
      if (!ImGui::GetIO().WantCaptureKeyboard) {
        char c = 0;
        SDL_Keycode key = event.key.keysym.sym;
        if (key == SDLK_RETURN)
          c = '\n';
        else if (key == SDLK_BACKSPACE)
          c = '\b';
        else if (key == SDLK_ESCAPE)
          quit_requested = true;
        else if (key >= SDLK_SPACE && key <= SDLK_z)
          c = (char)key;

        if (c != 0) {
          std::lock_guard<std::mutex> lock(key_mutex);
          key_buffer += c;
        }
      }
      break;
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
  // We can use ImGui for this now too!
  std::stringstream ss;
  ss << "AR1 VCPU | " << mips << " MIPS";
  set_title(ss.str());
}

} // namespace ar1
