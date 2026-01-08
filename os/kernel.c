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

  uart_puts("\n[VCPU] Init GIC...\n");
  gic_init();
  uart_puts("[VCPU] Init Timer...\n");
  timer_init();

  uart_puts("[VCPU] Init VirtIO...\n");
  virtio_init();
  virtio_read_sector(0);

  // Unmask IRQ
  asm volatile("msr daifclr, #2");

  uart_puts("[VCPU] Calculator Ready (with Ticks).\n> ");

  char buf[64];
  int idx = 0;

  while (1) {
    char c = uart_getc();
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
