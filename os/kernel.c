typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;
typedef int i32;

// ============================================================================
// HARDWARE DEFINITIONS
// ============================================================================

#define UART_BASE 0x09000000
#define UART_DR (*((volatile u32 *)(UART_BASE + 0x00)))
#define UART_FR (*((volatile u32 *)(UART_BASE + 0x18)))

#define GIC_DIST_BASE 0x08000000
#define GIC_CPU_BASE 0x08100000
#define GICD_CTLR (*((volatile u32 *)(GIC_DIST_BASE + 0x000)))
#define GICD_ISENABLER (*((volatile u32 *)(GIC_DIST_BASE + 0x100)))
#define GICC_CTLR (*((volatile u32 *)(GIC_CPU_BASE + 0x000)))
#define GICC_PMR (*((volatile u32 *)(GIC_CPU_BASE + 0x004)))
#define GICC_IAR (*((volatile u32 *)(GIC_CPU_BASE + 0x00C)))
#define GICC_EOIR (*((volatile u32 *)(GIC_CPU_BASE + 0x010)))

#define FB_BASE 0x0B000000
#define FB_WIDTH (*((volatile u32 *)(FB_BASE + 0x00)))
#define FB_HEIGHT (*((volatile u32 *)(FB_BASE + 0x04)))
#define FB_FLUSH (*((volatile u32 *)(FB_BASE + 0x10)))
#define FB_PIXEL (*((volatile u32 *)(FB_BASE + 0x14)))
#define FB_COLOR (*((volatile u32 *)(FB_BASE + 0x18)))
#define FB_FILL (*((volatile u32 *)(FB_BASE + 0x1C)))

#define MOUSE_BASE 0x0C000000
#define MOUSE_X (*((volatile u32 *)(MOUSE_BASE + 0x00)))
#define MOUSE_Y (*((volatile u32 *)(MOUSE_BASE + 0x04)))
#define MOUSE_BUTTONS (*((volatile u32 *)(MOUSE_BASE + 0x08)))
#define MOUSE_PRESENT (*((volatile u32 *)(MOUSE_BASE + 0x0C)))

#define NET_BASE 0x0D000000
#define NET_STATUS (*((volatile u32 *)(NET_BASE + 0x00)))
#define NET_CMD (*((volatile u32 *)(NET_BASE + 0x04)))
#define NET_TX_CHAR (*((volatile u32 *)(NET_BASE + 0x0C)))
#define NET_RX_CHAR (*((volatile u32 *)(NET_BASE + 0x10)))
#define NET_CLEAR (*((volatile u32 *)(NET_BASE + 0x14)))

// Shared memory for multi-core communication (must be early for visibility)
static volatile u32 core1_counter = 0;
static volatile u32 core1_active = 0;

// ============================================================================
// BASIC I/O
// ============================================================================

void uart_putc(char c) { UART_DR = c; }
void uart_puts(const char *s) {
  while (*s)
    uart_putc(*s++);
}
char uart_trygetc() {
  if (UART_FR & 0x10)
    return 0;
  return (char)(UART_DR & 0xFF);
}

// ============================================================================
// FRAMEBUFFER
// ============================================================================

static u32 fb_width = 320, fb_height = 200;

void fb_init() {
  fb_width = FB_WIDTH;
  fb_height = FB_HEIGHT;
}
void fb_flush() { FB_FLUSH = 1; }
void fb_fill(u32 color) {
  FB_COLOR = color;
  FB_FILL = 1;
}
void fb_set_pixel(u32 x, u32 y, u32 color) {
  if (x < fb_width && y < fb_height) {
    FB_PIXEL = (x & 0xFFFF) | ((y & 0xFFFF) << 16);
    FB_COLOR = color;
  }
}

void fb_draw_rect(u32 x, u32 y, u32 w, u32 h, u32 color) {
  for (u32 j = y; j < y + h && j < fb_height; j++)
    for (u32 i = x; i < x + w && i < fb_width; i++)
      fb_set_pixel(i, j, color);
}

void fb_draw_char(u32 x, u32 y, char c, u32 color) {
  // Simple 5x7 font (ASCII 32-127)
  static const u8 font[96][5] = {
      {0x00, 0x00, 0x00, 0x00, 0x00}, // space
      {0x00, 0x00, 0x5F, 0x00, 0x00}, // !
      {0x00, 0x07, 0x00, 0x07, 0x00}, // "
      {0x14, 0x7F, 0x14, 0x7F, 0x14}, // #
      {0x24, 0x2A, 0x7F, 0x2A, 0x12}, // $
      {0x23, 0x13, 0x08, 0x64, 0x62}, // %
      {0x36, 0x49, 0x55, 0x22, 0x50}, // &
      {0x00, 0x05, 0x03, 0x00, 0x00}, // '
      {0x00, 0x1C, 0x22, 0x41, 0x00}, // (
      {0x00, 0x41, 0x22, 0x1C, 0x00}, // )
      {0x08, 0x2A, 0x1C, 0x2A, 0x08}, // *
      {0x08, 0x08, 0x3E, 0x08, 0x08}, // +
      {0x00, 0x50, 0x30, 0x00, 0x00}, // ,
      {0x08, 0x08, 0x08, 0x08, 0x08}, // -
      {0x00, 0x60, 0x60, 0x00, 0x00}, // .
      {0x20, 0x10, 0x08, 0x04, 0x02}, // /
      {0x3E, 0x51, 0x49, 0x45, 0x3E}, // 0
      {0x00, 0x42, 0x7F, 0x40, 0x00}, // 1
      {0x42, 0x61, 0x51, 0x49, 0x46}, // 2
      {0x21, 0x41, 0x45, 0x4B, 0x31}, // 3
      {0x18, 0x14, 0x12, 0x7F, 0x10}, // 4
      {0x27, 0x45, 0x45, 0x45, 0x39}, // 5
      {0x3C, 0x4A, 0x49, 0x49, 0x30}, // 6
      {0x01, 0x71, 0x09, 0x05, 0x03}, // 7
      {0x36, 0x49, 0x49, 0x49, 0x36}, // 8
      {0x06, 0x49, 0x49, 0x29, 0x1E}, // 9
      {0x00, 0x36, 0x36, 0x00, 0x00}, // :
      {0x00, 0x56, 0x36, 0x00, 0x00}, // ;
      {0x00, 0x08, 0x14, 0x22, 0x41}, // <
      {0x14, 0x14, 0x14, 0x14, 0x14}, // =
      {0x41, 0x22, 0x14, 0x08, 0x00}, // >
      {0x02, 0x01, 0x51, 0x09, 0x06}, // ?
      {0x32, 0x49, 0x79, 0x41, 0x3E}, // @
      {0x7E, 0x11, 0x11, 0x11, 0x7E}, // A
      {0x7F, 0x49, 0x49, 0x49, 0x36}, // B
      {0x3E, 0x41, 0x41, 0x41, 0x22}, // C
      {0x7F, 0x41, 0x41, 0x22, 0x1C}, // D
      {0x7F, 0x49, 0x49, 0x49, 0x41}, // E
      {0x7F, 0x09, 0x09, 0x01, 0x01}, // F
      {0x3E, 0x41, 0x41, 0x51, 0x32}, // G
      {0x7F, 0x08, 0x08, 0x08, 0x7F}, // H
      {0x00, 0x41, 0x7F, 0x41, 0x00}, // I
      {0x20, 0x40, 0x41, 0x3F, 0x01}, // J
      {0x7F, 0x08, 0x14, 0x22, 0x41}, // K
      {0x7F, 0x40, 0x40, 0x40, 0x40}, // L
      {0x7F, 0x02, 0x04, 0x02, 0x7F}, // M
      {0x7F, 0x04, 0x08, 0x10, 0x7F}, // N
      {0x3E, 0x41, 0x41, 0x41, 0x3E}, // O
      {0x7F, 0x09, 0x09, 0x09, 0x06}, // P
      {0x3E, 0x41, 0x51, 0x21, 0x5E}, // Q
      {0x7F, 0x09, 0x19, 0x29, 0x46}, // R
      {0x46, 0x49, 0x49, 0x49, 0x31}, // S
      {0x01, 0x01, 0x7F, 0x01, 0x01}, // T
      {0x3F, 0x40, 0x40, 0x40, 0x3F}, // U
      {0x1F, 0x20, 0x40, 0x20, 0x1F}, // V
      {0x7F, 0x20, 0x18, 0x20, 0x7F}, // W
      {0x63, 0x14, 0x08, 0x14, 0x63}, // X
      {0x03, 0x04, 0x78, 0x04, 0x03}, // Y
      {0x61, 0x51, 0x49, 0x45, 0x43}, // Z
      {0x00, 0x00, 0x7F, 0x41, 0x41}, // [
      {0x02, 0x04, 0x08, 0x10, 0x20}, // backslash
      {0x41, 0x41, 0x7F, 0x00, 0x00}, // ]
      {0x04, 0x02, 0x01, 0x02, 0x04}, // ^
      {0x40, 0x40, 0x40, 0x40, 0x40}, // _
      {0x00, 0x01, 0x02, 0x04, 0x00}, // `
      {0x20, 0x54, 0x54, 0x54, 0x78}, // a
      {0x7F, 0x48, 0x44, 0x44, 0x38}, // b
      {0x38, 0x44, 0x44, 0x44, 0x20}, // c
      {0x38, 0x44, 0x44, 0x48, 0x7F}, // d
      {0x38, 0x54, 0x54, 0x54, 0x18}, // e
      {0x08, 0x7E, 0x09, 0x01, 0x02}, // f
      {0x08, 0x14, 0x54, 0x54, 0x3C}, // g
      {0x7F, 0x08, 0x04, 0x04, 0x78}, // h
      {0x00, 0x44, 0x7D, 0x40, 0x00}, // i
      {0x20, 0x40, 0x44, 0x3D, 0x00}, // j
      {0x00, 0x7F, 0x10, 0x28, 0x44}, // k
      {0x00, 0x41, 0x7F, 0x40, 0x00}, // l
      {0x7C, 0x04, 0x18, 0x04, 0x78}, // m
      {0x7C, 0x08, 0x04, 0x04, 0x78}, // n
      {0x38, 0x44, 0x44, 0x44, 0x38}, // o
      {0x7C, 0x14, 0x14, 0x14, 0x08}, // p
      {0x08, 0x14, 0x14, 0x18, 0x7C}, // q
      {0x7C, 0x08, 0x04, 0x04, 0x08}, // r
      {0x48, 0x54, 0x54, 0x54, 0x20}, // s
      {0x04, 0x3F, 0x44, 0x40, 0x20}, // t
      {0x3C, 0x40, 0x40, 0x20, 0x7C}, // u
      {0x1C, 0x20, 0x40, 0x20, 0x1C}, // v
      {0x3C, 0x40, 0x30, 0x40, 0x3C}, // w
      {0x44, 0x28, 0x10, 0x28, 0x44}, // x
      {0x0C, 0x50, 0x50, 0x50, 0x3C}, // y
      {0x44, 0x64, 0x54, 0x4C, 0x44}, // z
      {0x00, 0x08, 0x36, 0x41, 0x00}, // {
      {0x00, 0x00, 0x7F, 0x00, 0x00}, // |
      {0x00, 0x41, 0x36, 0x08, 0x00}, // }
      {0x08, 0x08, 0x2A, 0x1C, 0x08}, // ~
      {0x08, 0x1C, 0x2A, 0x08, 0x08}, // DEL (arrow)
  };
  if (c < 32 || c > 127)
    return;
  const u8 *glyph = font[c - 32];
  for (int i = 0; i < 5; i++) {
    for (int j = 0; j < 7; j++) {
      if (glyph[i] & (1 << j)) {
        fb_set_pixel(x + i, y + j, color);
      }
    }
  }
}

void fb_draw_string(u32 x, u32 y, const char *s, u32 color) {
  while (*s) {
    fb_draw_char(x, y, *s++, color);
    x += 6;
  }
}

// ============================================================================
// SIMPLE RANDOM
// ============================================================================

static u32 rand_seed = 12345;
u32 rand() {
  rand_seed = rand_seed * 1103515245 + 12345;
  return (rand_seed >> 16) & 0x7FFF;
}

// ============================================================================
// SNAKE GAME
// ============================================================================

#define SNAKE_SIZE 8
#define GRID_W (320 / SNAKE_SIZE)
#define GRID_H (200 / SNAKE_SIZE)
#define MAX_SNAKE 100

static i32 snake_x[MAX_SNAKE], snake_y[MAX_SNAKE];
static i32 snake_len = 3;
static i32 snake_dir = 0; // 0=right, 1=down, 2=left, 3=up
static i32 food_x, food_y;
static int game_over = 0;
static u32 score = 0;

void snake_spawn_food() {
  food_x = rand() % (GRID_W - 2) + 1;
  food_y = rand() % (GRID_H - 2) + 1;
}

void snake_init() {
  snake_len = 3;
  snake_dir = 0;
  game_over = 0;
  score = 0;
  for (int i = 0; i < snake_len; i++) {
    snake_x[i] = 10 - i;
    snake_y[i] = 10;
  }
  snake_spawn_food();
}

void snake_draw() {
  // Clear
  fb_fill(0x00000000);

  // Draw border
  for (u32 x = 0; x < GRID_W; x++) {
    fb_draw_rect(x * SNAKE_SIZE, 0, SNAKE_SIZE, SNAKE_SIZE, 0x00404040);
    fb_draw_rect(x * SNAKE_SIZE, (GRID_H - 1) * SNAKE_SIZE, SNAKE_SIZE,
                 SNAKE_SIZE, 0x00404040);
  }
  for (u32 y = 0; y < GRID_H; y++) {
    fb_draw_rect(0, y * SNAKE_SIZE, SNAKE_SIZE, SNAKE_SIZE, 0x00404040);
    fb_draw_rect((GRID_W - 1) * SNAKE_SIZE, y * SNAKE_SIZE, SNAKE_SIZE,
                 SNAKE_SIZE, 0x00404040);
  }

  // Draw food
  fb_draw_rect(food_x * SNAKE_SIZE + 1, food_y * SNAKE_SIZE + 1, SNAKE_SIZE - 2,
               SNAKE_SIZE - 2, 0x00FF0000);

  // Draw snake
  for (int i = 0; i < snake_len; i++) {
    u32 color = (i == 0) ? 0x0000FF00 : 0x0000AA00; // Head is brighter
    fb_draw_rect(snake_x[i] * SNAKE_SIZE + 1, snake_y[i] * SNAKE_SIZE + 1,
                 SNAKE_SIZE - 2, SNAKE_SIZE - 2, color);
  }

  // Draw score
  fb_draw_string(10, 2, "SCORE:", 0x00FFFFFF);
  char sc[8];
  sc[0] = '0' + (score / 10) % 10;
  sc[1] = '0' + score % 10;
  sc[2] = 0;
  fb_draw_string(50, 2, sc, 0x00FFFF00);

  if (game_over) {
    fb_draw_string(120, 90, "GAME OVER", 0x00FF0000);
    fb_draw_string(100, 100, "Press SPACE to restart", 0x00FFFFFF);
  }

  fb_flush();
}

void snake_update() {
  if (game_over)
    return;

  // Move body
  for (int i = snake_len - 1; i > 0; i--) {
    snake_x[i] = snake_x[i - 1];
    snake_y[i] = snake_y[i - 1];
  }

  // Move head
  switch (snake_dir) {
  case 0:
    snake_x[0]++;
    break;
  case 1:
    snake_y[0]++;
    break;
  case 2:
    snake_x[0]--;
    break;
  case 3:
    snake_y[0]--;
    break;
  }

  // Check wall collision
  if (snake_x[0] <= 0 || snake_x[0] >= GRID_W - 1 || snake_y[0] <= 0 ||
      snake_y[0] >= GRID_H - 1) {
    game_over = 1;
    return;
  }

  // Check self collision
  for (int i = 1; i < snake_len; i++) {
    if (snake_x[0] == snake_x[i] && snake_y[0] == snake_y[i]) {
      game_over = 1;
      return;
    }
  }

  // Check food
  if (snake_x[0] == food_x && snake_y[0] == food_y) {
    if (snake_len < MAX_SNAKE)
      snake_len++;
    score++;
    snake_spawn_food();
  }
}

void snake_input(char key) {
  if (game_over && key == ' ') {
    snake_init();
    return;
  }
  // Arrow keys or WASD
  if ((key == 'w' || key == 'W') && snake_dir != 1)
    snake_dir = 3;
  else if ((key == 's' || key == 'S') && snake_dir != 3)
    snake_dir = 1;
  else if ((key == 'a' || key == 'A') && snake_dir != 0)
    snake_dir = 2;
  else if ((key == 'd' || key == 'D') && snake_dir != 2)
    snake_dir = 0;
}

// ============================================================================
// GUI SYSTEM
// ============================================================================

#define MAX_WINDOWS 4

typedef struct {
  i32 x, y, w, h;
  const char *title;
  int visible;
  int dragging;
  int drag_ox, drag_oy;
} Window;

static Window windows[MAX_WINDOWS];
static int active_window = -1;
static int current_app = 0; // 0=desktop, 1=snake, 2=about, 3=calc

void gui_init() {
  // Calculator window
  windows[0] = (Window){50, 50, 120, 80, "Calculator", 1, 0, 0, 0};
  // About window
  windows[1] = (Window){100, 60, 140, 60, "About AR1", 1, 0, 0, 0};
  // Snake window
  windows[2] = (Window){20, 30, 150, 100, "Snake Game", 0, 0, 0, 0};
  active_window = 0;
}

void gui_draw_window(Window *w, int is_active) {
  if (!w->visible)
    return;

  // Shadow
  fb_draw_rect(w->x + 3, w->y + 3, w->w, w->h, 0x00202020);

  // Window body
  fb_draw_rect(w->x, w->y, w->w, w->h, 0x00E0E0E0);

  // Title bar
  u32 title_color = is_active ? 0x003366FF : 0x00808080;
  fb_draw_rect(w->x, w->y, w->w, 12, title_color);

  // Title text
  fb_draw_string(w->x + 4, w->y + 3, w->title, 0x00FFFFFF);

  // Close button
  fb_draw_rect(w->x + w->w - 10, w->y + 2, 8, 8, 0x00FF4444);
}

void gui_draw_taskbar() {
  // Taskbar at bottom
  fb_draw_rect(0, 188, 320, 12, 0x00333333);

  // App buttons
  fb_draw_rect(5, 190, 40, 8, 0x00555555);
  fb_draw_string(8, 191, "Calc", 0x00FFFFFF);

  fb_draw_rect(50, 190, 40, 8, 0x00555555);
  fb_draw_string(53, 191, "About", 0x00FFFFFF);

  fb_draw_rect(95, 190, 40, 8, 0x00555555);
  fb_draw_string(98, 191, "Snake", 0x00FFFFFF);

  // Clock area
  fb_draw_string(280, 191, "AR1", 0x0000FF00);
}

void gui_draw_desktop() {
  // Desktop background
  fb_fill(0x00003366);

  // Draw all windows
  for (int i = 0; i < MAX_WINDOWS; i++) {
    gui_draw_window(&windows[i], i == active_window);
  }

  // Content for Calculator window
  if (windows[0].visible) {
    fb_draw_string(windows[0].x + 10, windows[0].y + 20,
                   "Type expression:", 0x00000000);
    fb_draw_string(windows[0].x + 10, windows[0].y + 35, "e.g. 5+3",
                   0x00666666);
  }

  // Content for About window - show Core 1 status
  if (windows[1].visible) {
    fb_draw_string(windows[1].x + 10, windows[1].y + 20, "AR1 VCPU v2.3",
                   0x00000000);
    fb_draw_string(windows[1].x + 10, windows[1].y + 35, "2-Core SMP",
                   0x00666666);

    // Show Core 1 counter
    char cnt[16];
    u32 c = core1_counter / 1000;
    cnt[0] = 'C';
    cnt[1] = '1';
    cnt[2] = ':';
    cnt[3] = '0' + (c / 100) % 10;
    cnt[4] = '0' + (c / 10) % 10;
    cnt[5] = '0' + c % 10;
    cnt[6] = 'K';
    cnt[7] = 0;
    fb_draw_string(windows[1].x + 10, windows[1].y + 47, cnt,
                   core1_active ? 0x0000AA00 : 0x00AA0000);
  }

  gui_draw_taskbar();
  fb_flush();
}

void gui_handle_mouse(u32 mx, u32 my, u32 buttons) {
  static u32 prev_buttons = 0;
  int just_clicked = (buttons & 1) && !(prev_buttons & 1);
  prev_buttons = buttons;

  // Check taskbar clicks
  if (just_clicked && my >= 188) {
    if (mx >= 5 && mx < 45) {
      windows[0].visible = 1;
      active_window = 0;
      current_app = 0;
    } else if (mx >= 50 && mx < 90) {
      windows[1].visible = 1;
      active_window = 1;
      current_app = 0;
    } else if (mx >= 95 && mx < 135) {
      current_app = 1;
      snake_init();
    } // Start Snake
    return;
  }

  // Dragging
  for (int i = MAX_WINDOWS - 1; i >= 0; i--) {
    Window *w = &windows[i];
    if (!w->visible)
      continue;

    if (buttons & 1) {
      if (w->dragging) {
        w->x = mx - w->drag_ox;
        w->y = my - w->drag_oy;
        if (w->x < 0)
          w->x = 0;
        if (w->y < 0)
          w->y = 0;
        if (w->x > 320 - w->w)
          w->x = 320 - w->w;
        if (w->y > 188 - w->h)
          w->y = 188 - w->h;
      } else if (just_clicked) {
        // Check title bar
        if (mx >= w->x && mx < w->x + w->w && my >= w->y && my < w->y + 12) {
          // Check close button
          if (mx >= w->x + w->w - 10) {
            w->visible = 0;
          } else {
            w->dragging = 1;
            w->drag_ox = mx - w->x;
            w->drag_oy = my - w->y;
            active_window = i;
          }
          break;
        }
      }
    } else {
      w->dragging = 0;
    }
  }
}

// ============================================================================
// GIC & TIMER (simplified)
// ============================================================================

void gic_init() {
  GICD_CTLR = 1;
  GICD_ISENABLER = 0xFFFFFFFF;
  GICC_CTLR = 1;
  GICC_PMR = 0xFF;
}

#define ARM_SYSREG_WRITE(REG, VAL)                                             \
  asm volatile("msr " #REG ", %0" ::"r"((u64)(VAL)))
#define ARM_SYSREG_READ(REG)                                                   \
  ({                                                                           \
    u64 _v;                                                                    \
    asm volatile("mrs %0, " #REG : "=r"(_v));                                  \
    _v;                                                                        \
  })

void timer_init() {
  u64 freq = ARM_SYSREG_READ(cntfrq_el0);
  ARM_SYSREG_WRITE(cntv_tval_el0, freq / 10); // 100ms
  ARM_SYSREG_WRITE(cntv_ctl_el0, (u64)1);
}

void timer_handler() {
  u64 freq = ARM_SYSREG_READ(cntfrq_el0);
  ARM_SYSREG_WRITE(cntv_tval_el0, freq / 10);
}

void c_irq_handler() {
  u32 iar = GICC_IAR;
  u32 irq = iar & 0x3FF;
  if (irq == 27)
    timer_handler();
  GICC_EOIR = iar;
}

// ============================================================================
// KERNEL MAIN
// ============================================================================

void core1_main() {
  core1_active = 1;
  while (1) {
    core1_counter++;
    // Small delay
    for (volatile int i = 0; i < 100000; i++)
      ;
  }
}

void kernel_main() {
  u64 mpidr;
  asm volatile("mrs %0, mpidr_el1" : "=r"(mpidr));
  int core_id = mpidr & 0xFF;

  // Secondary cores run their own task
  if (core_id != 0) {
    core1_main();
    while (1)
      asm volatile("wfi");
  }

  uart_puts("\n========================================\n");
  uart_puts("         AR1 VCPU v2.3 by APRK\n");
  uart_puts("    Multi-Core SMP Edition (2 Cores)\n");
  uart_puts("========================================\n\n");

  gic_init();
  timer_init();
  fb_init();
  gui_init();
  snake_init();

  uart_puts("[VCPU] GUI OS Ready!\n");
  uart_puts("[VCPU] Core 0: Main GUI\n");
  uart_puts("[VCPU] Core 1: Background counter\n\n");
  uart_puts("Controls:\n");
  uart_puts("  - Click taskbar buttons to switch apps\n");
  uart_puts("  - Drag window title bars\n");
  uart_puts("  - Snake: WASD to move, SPACE to restart\n\n");

  asm volatile("msr daifclr, #2"); // Enable IRQ

  u32 frame = 0;

  while (1) {
    u32 mx = MOUSE_X;
    u32 my = MOUSE_Y;
    u32 buttons = MOUSE_BUTTONS;
    char key = uart_trygetc();

    if (current_app == 1) {
      // Snake Game
      if (key)
        snake_input(key);
      if (frame % 5 == 0) { // Slow down snake
        snake_update();
      }
      snake_draw();

      // ESC to exit to desktop
      if (key == 27 || key == 'q' || key == 'Q') {
        current_app = 0;
      }
    } else {
      // Desktop GUI
      if (MOUSE_PRESENT) {
        gui_handle_mouse(mx, my, buttons);
      }
      gui_draw_desktop();

      // Draw mouse cursor
      if (MOUSE_PRESENT) {
        fb_draw_rect(mx, my, 6, 6, 0x00FFFFFF);
        fb_flush();
      }
    }

    frame++;
    // Small delay
    for (volatile int i = 0; i < 50000; i++)
      ;
  }
}
