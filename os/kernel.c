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

char uart_getc() {
  while (UART_FR & 0x10) {
  } // Spin
  return (char)(UART_DR & 0xFF);
}

// Non-blocking version - returns 0 if no char available
char uart_trygetc() {
  if (UART_FR & 0x10) {
    return 0; // No data available
  }
  return (char)(UART_DR & 0xFF);
}

void print_dec(long long val) {
  char buf[32];
  int i = 0;
  if (val == 0) {
    uart_putc('0');
    return;
  }
  if (val < 0) {
    uart_putc('-');
    val = -val;
  }
  while (val) {
    buf[i++] = '0' + (val % 10);
    val /= 10;
  }
  while (i > 0)
    uart_putc(buf[--i]);
}

// GIC
#define GIC_DIST_BASE 0x08000000
#define GIC_CPU_BASE 0x08100000

#define GICD_CTLR (*((volatile u32 *)(GIC_DIST_BASE + 0x000)))
#define GICD_ISENABLER (*((volatile u32 *)(GIC_DIST_BASE + 0x100)))

#define GICC_CTLR (*((volatile u32 *)(GIC_CPU_BASE + 0x000)))
#define GICC_PMR (*((volatile u32 *)(GIC_CPU_BASE + 0x004)))
#define GICC_IAR (*((volatile u32 *)(GIC_CPU_BASE + 0x00C)))
#define GICC_EOIR (*((volatile u32 *)(GIC_CPU_BASE + 0x010)))

// VIRTIO
#define VIRTIO_BASE 0x0A000000
#define VIRTIO_MAGIC_VALUE (*((volatile u32 *)(VIRTIO_BASE + 0x000)))
#define VIRTIO_VERSION (*((volatile u32 *)(VIRTIO_BASE + 0x004)))
#define VIRTIO_DEVICE_ID (*((volatile u32 *)(VIRTIO_BASE + 0x008)))
#define VIRTIO_STATUS (*((volatile u32 *)(VIRTIO_BASE + 0x070)))
#define VIRTIO_QUEUE_NUM (*((volatile u32 *)(VIRTIO_BASE + 0x038)))
#define VIRTIO_QUEUE_SEL (*((volatile u32 *)(VIRTIO_BASE + 0x030)))
#define VIRTIO_QUEUE_READY (*((volatile u32 *)(VIRTIO_BASE + 0x044)))
#define VIRTIO_QUEUE_NOTIFY (*((volatile u32 *)(VIRTIO_BASE + 0x050)))
#define VIRTIO_QUEUE_DESC_LOW (*((volatile u32 *)(VIRTIO_BASE + 0x080)))
#define VIRTIO_QUEUE_DESC_HIGH (*((volatile u32 *)(VIRTIO_BASE + 0x084)))
#define VIRTIO_QUEUE_DRIVER_LOW (*((volatile u32 *)(VIRTIO_BASE + 0x090)))
#define VIRTIO_QUEUE_DRIVER_HIGH (*((volatile u32 *)(VIRTIO_BASE + 0x094)))
#define VIRTIO_QUEUE_DEVICE_LOW (*((volatile u32 *)(VIRTIO_BASE + 0x0A0)))
#define VIRTIO_QUEUE_DEVICE_HIGH (*((volatile u32 *)(VIRTIO_BASE + 0x0A4)))

// Framebuffer
#define FB_BASE 0x0B000000
#define FB_WIDTH (*((volatile u32 *)(FB_BASE + 0x00)))
#define FB_HEIGHT (*((volatile u32 *)(FB_BASE + 0x04)))
#define FB_ENABLED (*((volatile u32 *)(FB_BASE + 0x0C)))
#define FB_FLUSH (*((volatile u32 *)(FB_BASE + 0x10)))
#define FB_PIXEL (*((volatile u32 *)(FB_BASE + 0x14)))
#define FB_COLOR (*((volatile u32 *)(FB_BASE + 0x18)))
#define FB_FILL (*((volatile u32 *)(FB_BASE + 0x1C)))

// Mouse Device
#define MOUSE_BASE 0x0C000000
#define MOUSE_X (*((volatile u32 *)(MOUSE_BASE + 0x00)))
#define MOUSE_Y (*((volatile u32 *)(MOUSE_BASE + 0x04)))
#define MOUSE_BUTTONS (*((volatile u32 *)(MOUSE_BASE + 0x08)))
#define MOUSE_PRESENT (*((volatile u32 *)(MOUSE_BASE + 0x0C)))

// Network Device
#define NET_BASE 0x0D000000
#define NET_STATUS (*((volatile u32 *)(NET_BASE + 0x00)))
#define NET_CMD (*((volatile u32 *)(NET_BASE + 0x04)))
#define NET_DATA_LEN (*((volatile u32 *)(NET_BASE + 0x08)))
#define NET_TX_CHAR (*((volatile u32 *)(NET_BASE + 0x0C)))
#define NET_RX_CHAR (*((volatile u32 *)(NET_BASE + 0x10)))
#define NET_CLEAR (*((volatile u32 *)(NET_BASE + 0x14)))

#define NET_IDLE 0
#define NET_BUSY 1
#define NET_DONE 2
#define NET_ERROR 3

#define NET_CMD_PING 1
#define NET_CMD_FETCH 2

// VirtIO Structures in memory
#define RING_SIZE 128

struct vring_desc {
  u64 addr;
  u32 len;
  u16 flags;
  u16 next;
};

struct vring_avail {
  u16 flags;
  u16 idx;
  u16 ring[RING_SIZE];
};

struct vring_used_elem {
  u32 id;
  u32 len;
};

struct vring_used {
  u16 flags;
  u16 idx;
  struct vring_used_elem ring[RING_SIZE];
};

// Memory for Vring (fixed address for simplicity)
struct vring_desc *desc_table = (struct vring_desc *)0x41000000;
struct vring_avail *avail_ring = (struct vring_avail *)0x41001000;
struct vring_used *used_ring = (struct vring_used *)0x41002000;

struct virtio_blk_req {
  u32 type;
  u32 reserved;
  u64 sector;
};

// Buffers
struct virtio_blk_req *req_header = (struct virtio_blk_req *)0x41003000;
u8 *disk_buffer = (u8 *)0x41004000;
u8 *req_status = (u8 *)0x41005000;

// Helper
#define ARM_SYSREG_READ(REG)                                                   \
  ({                                                                           \
    u64 _val;                                                                  \
    asm volatile("mrs %0, " #REG : "=r"(_val));                                \
    _val;                                                                      \
  })
#define ARM_SYSREG_WRITE(REG, VAL) asm volatile("msr " #REG ", %0" ::"r"(VAL))

void gic_init() {
  GICD_CTLR = 1;
  GICC_CTLR = 1;
  GICC_PMR = 0xF0;
  GICD_ISENABLER = (1 << 27); // Timer ID 27
}

void timer_init() {
  u64 freq = ARM_SYSREG_READ(cntfrq_el0);
  ARM_SYSREG_WRITE(cntv_tval_el0, freq / 10);
  ARM_SYSREG_WRITE(cntv_ctl_el0, 1);
}

// Framebuffer Graphics
void fb_init() {
  FB_ENABLED = 1;
  uart_puts("[FB] Enabled\n");
}

void fb_set_pixel(u32 x, u32 y, u32 color) {
  FB_PIXEL = (x << 16) | (y & 0xFFFF);
  FB_COLOR = color;
}

void fb_fill(u32 color) { FB_FILL = color; }

void fb_draw_rect(u32 x, u32 y, u32 w, u32 h, u32 color) {
  for (u32 dy = 0; dy < h; dy++) {
    for (u32 dx = 0; dx < w; dx++) {
      fb_set_pixel(x + dx, y + dy, color);
    }
  }
}

void fb_flush() { FB_FLUSH = 1; }

void virtio_init() {
  if (VIRTIO_MAGIC_VALUE != 0x74726976) {
    uart_puts("VirtIO: Bad Magic\n");
    return;
  }
  if (VIRTIO_DEVICE_ID != 2) {
    uart_puts("VirtIO: Not Block Device\n");
    return;
  }

  // Reset
  VIRTIO_STATUS = 0;
  // Acknowledge
  VIRTIO_STATUS |= 1; // ACKNOWLEDGE
  VIRTIO_STATUS |= 2; // DRIVER
  VIRTIO_STATUS |= 4; // FEATURES_OK

  // Setup Queue 0
  VIRTIO_QUEUE_SEL = 0;
  VIRTIO_QUEUE_NUM = RING_SIZE;

  VIRTIO_QUEUE_DESC_LOW = (u64)desc_table;
  VIRTIO_QUEUE_DESC_HIGH = 0;
  VIRTIO_QUEUE_DRIVER_LOW = (u64)avail_ring;
  VIRTIO_QUEUE_DRIVER_HIGH = 0;
  VIRTIO_QUEUE_DEVICE_LOW = (u64)used_ring;
  VIRTIO_QUEUE_DEVICE_HIGH = 0;

  VIRTIO_QUEUE_READY = 1;

  VIRTIO_STATUS |= 4; // DRIVER_OK (Wait, 4 is features ok...)
  // Mask is: 1=ACK, 2=DRIVER, 4=FEATURES_OK, 8=DRIVER_OK
  VIRTIO_STATUS |= 8;

  uart_puts("VirtIO: Init Done.\n");
}

void virtio_read_sector(u64 sector) {
  // Fill Header
  req_header->type = 0; // IN (Read)
  req_header->reserved = 0;
  req_header->sector = sector;

  // Descriptor 0: Header
  desc_table[0].addr = (u64)req_header;
  desc_table[0].len = 16;
  desc_table[0].flags = 1; // NEXT
  desc_table[0].next = 1;

  // Descriptor 1: Buffer
  desc_table[1].addr = (u64)disk_buffer;
  desc_table[1].len = 512;
  desc_table[1].flags = 1 | 2; // NEXT | WRITE (Device writes to buffer)
  desc_table[1].next = 2;

  // Descriptor 2: Status
  desc_table[2].addr = (u64)req_status;
  desc_table[2].len = 1;
  desc_table[2].flags = 2; // WRITE
  desc_table[2].next = 0;

  // Update Available Ring
  avail_ring->ring[avail_ring->idx % RING_SIZE] = 0; // Head descriptor index

  // Memory Barrier would go here
  __atomic_thread_fence(__ATOMIC_SEQ_CST);

  avail_ring->idx++;

  __atomic_thread_fence(__ATOMIC_SEQ_CST);

  // Notify
  VIRTIO_QUEUE_NOTIFY = 0;

  // Spin wait for used
  // while (used_ring->idx == 0) {}  // Simplistic wait

  // Wait for a bit (simulate wait)
  for (volatile int i = 0; i < 100000; i++)
    ;

  uart_puts("Read Complete. First 32 chars:\n");
  for (int i = 0; i < 32; i++)
    uart_putc((char)disk_buffer[i]);
  uart_putc('\n');
}

// Handler
void c_irq_handler() {
  u32 iar = GICC_IAR;
  u32 id = iar & 0x3FF;

  if (id == 27) {
    // Timer
    uart_putc('!');

    // Reload Timer
    u64 freq = ARM_SYSREG_READ(cntfrq_el0);
    ARM_SYSREG_WRITE(cntv_tval_el0, freq / 10);
  }

  GICC_EOIR = iar;
}

// Calculator
long long parse_int(const char **p) {
  long long v = 0;
  while (**p == ' ')
    (*p)++;
  while (**p >= '0' && **p <= '9') {
    v = v * 10 + (**p - '0');
    (*p)++;
  }
  return v;
}

long long eval(const char *s) {
  const char *p = s;
  long long a = parse_int(&p);
  while (*p == ' ')
    p++;
  char op = *p;
  p++;
  long long b = parse_int(&p);
  if (op == '+')
    return a + b;
  if (op == '-')
    return a - b;
  if (op == '*')
    return a * b;
  if (op == '/')
    return (b != 0) ? (a / b) : 0;
  return a;
}

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

  uart_puts("[VCPU] Init GIC...\n");
  gic_init();
  uart_puts("[VCPU] Init Timer...\n");
  timer_init();

  uart_puts("[VCPU] Init VirtIO...\n");
  virtio_init();
  virtio_read_sector(0);

  uart_puts("[VCPU] Init Framebuffer...\n");
  fb_init();

  // Graphics Demo - Draw colorful pattern
  uart_puts("[VCPU] Drawing graphics demo...\n");

  // Fill screen with dark blue
  fb_fill(0x00102030);

  // Draw some colored rectangles
  fb_draw_rect(20, 20, 60, 40, 0x00FF0000);  // Red
  fb_draw_rect(100, 30, 60, 40, 0x0000FF00); // Green
  fb_draw_rect(180, 40, 60, 40, 0x000000FF); // Blue
  fb_draw_rect(260, 50, 40, 30, 0x00FFFF00); // Yellow

  // Draw a gradient bar at bottom
  for (u32 x = 0; x < 320; x++) {
    u32 r = x & 0xFF;
    u32 g = (320 - x) & 0xFF;
    u32 b = 128;
    u32 color = r | (g << 8) | (b << 16);
    for (u32 y = 180; y < 200; y++) {
      fb_set_pixel(x, y, color);
    }
  }

  // Draw border
  for (u32 x = 0; x < 320; x++) {
    fb_set_pixel(x, 0, 0x00FFFFFF);
    fb_set_pixel(x, 199, 0x00FFFFFF);
  }
  for (u32 y = 0; y < 200; y++) {
    fb_set_pixel(0, y, 0x00FFFFFF);
    fb_set_pixel(319, y, 0x00FFFFFF);
  }

  fb_flush();
  uart_puts("[VCPU] Graphics demo complete!\n");

  // Network Test - Ping Google DNS
  uart_puts("[VCPU] Testing Network - Pinging 8.8.8.8...\n");
  NET_CLEAR = 1;
  const char *host = "8.8.8.8";
  for (int i = 0; host[i]; i++) {
    NET_TX_CHAR = host[i];
  }
  NET_CMD = NET_CMD_PING;

  // Wait for result
  while (NET_STATUS == NET_BUSY) {
    for (volatile int i = 0; i < 10000; i++)
      ;
  }

  if (NET_STATUS == NET_DONE) {
    uart_puts("[NET] Ping result:\n");
    u32 len = NET_DATA_LEN;
    for (u32 i = 0; i < len && i < 200; i++) {
      char c = (char)NET_RX_CHAR;
      uart_putc(c);
    }
    uart_puts("\n");
  } else {
    uart_puts("[NET] Ping failed!\n");
  }

  // Unmask IRQ
  asm volatile("msr daifclr, #2");

  uart_puts("\n[VCPU] Mouse + Calculator Ready!\n");
  uart_puts("Move mouse over VCPU screen. Enter expressions:\n> ");

  char buf[64];
  int idx = 0;
  u32 last_mx = 0, last_my = 0;
  u32 cursor_color = 0x00FF00FF; // Magenta cursor

  while (1) {
    // Draw mouse cursor
    if (MOUSE_PRESENT) {
      u32 mx = MOUSE_X;
      u32 my = MOUSE_Y;

      // Erase old cursor (draw background color)
      if (last_mx != mx || last_my != my) {
        // Simple: just redraw a small region
        fb_draw_rect(last_mx, last_my, 8, 8, 0x00102030);
      }

      // Draw new cursor (simple 8x8 block)
      if (MOUSE_BUTTONS & 1) {
        cursor_color = 0x0000FF00; // Green when clicking
      } else {
        cursor_color = 0x00FF00FF; // Magenta
      }
      fb_draw_rect(mx, my, 8, 8, cursor_color);
      fb_flush();

      last_mx = mx;
      last_my = my;
    }

    // Handle keyboard (non-blocking)
    char c = uart_trygetc();
    if (c != 0) {
      uart_putc(c);

      if (c == '\n' || c == '\r') {
        buf[idx] = 0;
        long long res = eval(buf);
        uart_puts("\nResult: ");
        print_dec(res);
        uart_puts("\n> ");
        idx = 0;
      } else {
        if (idx < 63)
          buf[idx++] = c;
      }
    }
  }
}
