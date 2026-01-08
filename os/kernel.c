typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;

#define UART_BASE 0x09000000
#define UART_DR (*((volatile u32 *)(UART_BASE + 0x00)))
#define UART_FR (*((volatile u32 *)(UART_BASE + 0x18)))

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

void c_irq_handler() {}

// Framebuffer
#define FB_BASE 0x0B000000
#define FB_WIDTH (*((volatile u32 *)(FB_BASE + 0x00)))
#define FB_HEIGHT (*((volatile u32 *)(FB_BASE + 0x04)))
#define FB_FLUSH (*((volatile u32 *)(FB_BASE + 0x10)))
#define FB_PIXEL (*((volatile u32 *)(FB_BASE + 0x14)))
#define FB_COLOR (*((volatile u32 *)(FB_BASE + 0x18)))
#define FB_FILL (*((volatile u32 *)(FB_BASE + 0x1C)))

// Mouse
#define MOUSE_BASE 0x0C000000
#define MOUSE_X (*((volatile u32 *)(MOUSE_BASE + 0x00)))
#define MOUSE_Y (*((volatile u32 *)(MOUSE_BASE + 0x04)))
#define MOUSE_BUTTONS (*((volatile u32 *)(MOUSE_BASE + 0x08)))
#define MOUSE_PRESENT (*((volatile u32 *)(MOUSE_BASE + 0x0C)))

static u32 fb_width = 320, fb_height = 200;

#define RGB(r, g, b) ((r) | ((g) << 8) | ((b) << 16))

#define COLOR_BG RGB(0x10, 0x20, 0x40)
#define COLOR_CYAN RGB(0x00, 0xFF, 0xFF)
#define COLOR_WHITE RGB(0xFF, 0xFF, 0xFF)
#define COLOR_YELLOW RGB(0xFF, 0xFF, 0x00)
#define COLOR_GREEN RGB(0x00, 0xFF, 0x00)
#define COLOR_RED RGB(0xFF, 0x00, 0x00)
#define COLOR_GRAY RGB(0x60, 0x60, 0x60)

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
    FB_PIXEL = ((x & 0xFFFF) << 16) | (y & 0xFFFF);
    FB_COLOR = color;
  }
}

void fb_draw_rect(u32 x, u32 y, u32 w, u32 h, u32 color) {
  for (u32 j = y; j < y + h && j < fb_height; j++)
    for (u32 i = x; i < x + w && i < fb_width; i++)
      fb_set_pixel(i, j, color);
}

// Simple digit drawing (8x10 size)
void draw_digit(u32 x, u32 y, int digit, u32 color) {
  // 7-segment style
  int segs[10] = {
      0x3F, // 0: top, top-left, top-right, bottom-left, bottom-right, bottom
      0x06, // 1: top-right, bottom-right
      0x5B, // 2: top, top-right, middle, bottom-left, bottom
      0x4F, // 3: top, top-right, middle, bottom-right, bottom
      0x66, // 4: top-left, top-right, middle, bottom-right
      0x6D, // 5: top, top-left, middle, bottom-right, bottom
      0x7D, // 6: top, top-left, middle, bottom-left, bottom-right, bottom
      0x07, // 7: top, top-right, bottom-right
      0x7F, // 8: all
      0x6F, // 9: top, top-left, top-right, middle, bottom-right, bottom
  };

  if (digit < 0 || digit > 9)
    return;
  int s = segs[digit];

  // Segment positions (scaled 8x10)
  if (s & 0x01)
    fb_draw_rect(x + 1, y, 6, 2, color); // top
  if (s & 0x02)
    fb_draw_rect(x + 6, y + 1, 2, 4, color); // top-right
  if (s & 0x04)
    fb_draw_rect(x + 6, y + 5, 2, 4, color); // bottom-right
  if (s & 0x08)
    fb_draw_rect(x + 1, y + 8, 6, 2, color); // bottom
  if (s & 0x10)
    fb_draw_rect(x, y + 5, 2, 4, color); // bottom-left
  if (s & 0x20)
    fb_draw_rect(x, y + 1, 2, 4, color); // top-left
  if (s & 0x40)
    fb_draw_rect(x + 1, y + 4, 6, 2, color); // middle
}

// Draw a simple letter block
void draw_letter_block(u32 x, u32 y, char c, u32 color) {
  fb_draw_rect(x, y, 16, 20, color);
  // Draw the character as a pattern in the center
  u32 inner = (color == COLOR_WHITE) ? COLOR_CYAN : COLOR_WHITE;
  fb_draw_rect(x + 4, y + 4, 8, 12, inner);
}

// Text buffer
static char typed[64];
static int typed_len = 0;

void kernel_main() {
  u64 mpidr;
  asm volatile("mrs %0, mpidr_el1" : "=r"(mpidr));
  int core_id = mpidr & 0xFF;
  if (core_id != 0) {
    while (1)
      asm volatile("wfi");
  }

  uart_puts("\n========================================\n");
  uart_puts("         AR1 VCPU by APRK\n");
  uart_puts("========================================\n\n");
  uart_puts("[VCPU] Keyboard Demo Ready!\n");
  uart_puts("[VCPU] Type on keyboard, watch the screen!\n\n");

  fb_init();

  u32 frame = 0;

  while (1) {
    // Background
    fb_fill(COLOR_BG);

    // Title bar
    fb_draw_rect(0, 0, 320, 20, COLOR_GRAY);
    fb_draw_rect(10, 5, 100, 10, COLOR_CYAN); // "Title"

    // Instructions
    fb_draw_rect(20, 30, 280, 3, COLOR_WHITE);
    fb_draw_rect(60, 40, 200, 15, COLOR_CYAN);

    // Character count display
    fb_draw_rect(100, 70, 120, 40, COLOR_GRAY);
    draw_digit(110, 80, typed_len / 10, COLOR_GREEN);
    draw_digit(130, 80, typed_len % 10, COLOR_GREEN);

    // Typed characters visualization - show last 10 chars as blocks
    for (int i = 0; i < 10; i++) {
      int idx = typed_len - 10 + i;
      if (idx >= 0 && idx < typed_len) {
        u32 color = (i == 9) ? COLOR_YELLOW : COLOR_WHITE;
        fb_draw_rect(20 + i * 30, 130, 25, 30, color);
      } else {
        fb_draw_rect(20 + i * 30, 130, 25, 30, COLOR_GRAY);
      }
    }

    // Blinking cursor
    if ((frame / 15) % 2 == 0) {
      int cx = 20 + (typed_len % 10) * 30;
      fb_draw_rect(cx, 165, 25, 5, COLOR_YELLOW);
    }

    // Status bar
    fb_draw_rect(0, 180, 320, 20, COLOR_GRAY);
    fb_draw_rect(10, 185, 60, 10, COLOR_GREEN); // "Ready"

    fb_flush();

    // Handle keyboard input
    char c = uart_trygetc();
    if (c != 0) {
      uart_putc(c); // Echo to terminal

      if (c == 27) { // ESC - clear
        typed_len = 0;
        uart_puts("\n[Cleared]\n");
      } else if (c == 8 || c == 127) { // Backspace
        if (typed_len > 0) {
          typed_len--;
          uart_puts("\b \b");
        }
      } else if (c >= 32 && c < 127) { // Printable
        if (typed_len < 63) {
          typed[typed_len++] = c;
        }
      }
    }

    frame++;
    for (volatile int i = 0; i < 3000; i++)
      ;
  }
}
