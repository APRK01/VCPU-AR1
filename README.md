# AR1 VCPU

**A Virtual CPU by APRK**

A lightweight ARM64 virtual CPU for Apple Silicon Macs, built using Apple's Hypervisor Framework.

![Platform](https://img.shields.io/badge/platform-macOS%20(Apple%20Silicon)-blue)
![License](https://img.shields.io/badge/license-MIT-green)
![Language](https://img.shields.io/badge/language-C%2B%2B17-orange)

## Features

- **Native ARM64 Virtualization** - Uses Apple Hypervisor Framework for near-native performance
- **Interrupt Support** - GIC (Generic Interrupt Controller) with timer interrupts
- **VirtIO Storage** - VirtIO block device for disk I/O
- **Framebuffer Graphics** - 320x200 framebuffer with drawing primitives
- **UART Console** - Serial I/O for text-based interaction
- **Interactive Shell** - Debug and control the VCPU at runtime
- **Performance Monitoring** - Real-time MIPS/IPS statistics
- **Bare-Metal Kernel** - Includes a demo kernel with graphics, calculator, and more

## Requirements

- macOS 11.0+ (Big Sur or later)
- Apple Silicon Mac (M1/M2/M3/M4/M5)
- Xcode Command Line Tools
- CMake 3.16+

## Quick Start

### Build

```bash
# Clone the repository
git clone https://github.com/APRK01/VCPU-AR1.git
cd VCPU-AR1

# Build the kernel
./build_os.sh

# Build the emulator
mkdir -p build && cd build
cmake ..
make

# Create a test disk image (optional)
echo "Hello from AR1!" > disk.img
```

### Run

```bash
./build/ar1_vcpu kernel.bin
```

You'll see the AR1 interactive shell. Type `run` to start the VCPU:

```
===========================================
        AR1 INTERACTIVE SHELL             
===========================================
System Initialized. Cores Paused. type 'help' for commands.
> run

========================================
         AR1 VCPU by APRK
========================================

[VCPU] Init GIC...
[VCPU] Init Timer...
[VCPU] Init VirtIO...
VirtIO: Init Done.
[VCPU] Init Framebuffer...
[FB] Enabled
[VCPU] Drawing graphics demo...
[VCPU] Graphics demo complete!

[VCPU] Calculator Ready. Enter expression (e.g. 5+3):
> 
```

## Architecture

```
VCPU-AR1/
├── src/
│   ├── cpu/              # VCPU core (HVF integration)
│   ├── soc/              # System-on-Chip components
│   │   ├── bus.cpp       # Memory bus & MMIO routing
│   │   ├── gic.cpp       # Interrupt controller
│   │   ├── ram.cpp       # RAM management
│   │   └── uart.cpp      # Serial I/O
│   ├── device/           # Virtual devices
│   │   ├── virtio_blk.cpp    # VirtIO block storage
│   │   └── framebuffer.cpp   # 320x200 Framebuffer
│   └── main.cpp          # Entry point & shell
├── os/                   # Bare-metal kernel
│   ├── start.s           # Boot assembly
│   ├── kernel.c          # Kernel C code
│   └── linker.ld         # Linker script
├── include/              # Headers
└── build_os.sh           # Kernel build script
```

## Memory Map

| Address | Size | Description |
|---------|------|-------------|
| `0x09000000` | 4KB | UART (PL011-compatible) |
| `0x0A000000` | 512B | VirtIO Block Device |
| `0x0B000000` | 256B | Framebuffer MMIO |
| `0x08000000` | 64KB | GIC Distributor |
| `0x08100000` | 8KB | GIC CPU Interface |
| `0x40000000` | 64MB | RAM |
| `0xC0000000` | 8MB | VRAM |

## Shell Commands

| Command | Description |
|---------|-------------|
| `run` | Resume VCPU execution |
| `stop` | Pause VCPU execution |
| `regs` | Display CPU registers |
| `mem <addr>` | Dump memory at hex address |
| `perf` | Show performance statistics (MIPS, IPS) |
| `fb` | Save framebuffer to `fb_output.ppm` |
| `help` | Show available commands |
| `exit` | Exit emulator |

## Demo Kernel

The included kernel demonstrates:
- **Graphics** - Draws colorful rectangles and gradient bars
- **Timer Interrupts** - Prints `!` on each tick
- **VirtIO Disk Read** - Reads sector 0 from disk.img
- **Calculator** - Enter expressions like `5+3`, `10*2`, etc.

## Performance

The VCPU achieves approximately **0.2-0.3 MIPS** on Apple Silicon, running at near-native speeds thanks to hardware-accelerated virtualization.

Use the `perf` command to see real-time stats:
```
=== Performance Stats ===
Uptime: 15 seconds
Instructions: 3414605
IPS: 227640
MIPS: 0.23
=========================
```

## License

MIT License - see [LICENSE](LICENSE) file for details.

## Acknowledgments

- Apple Hypervisor Framework documentation
- VirtIO specification
- ARM Architecture Reference Manual

## Roadmap

- [x] Framebuffer/Graphics support
- [x] Performance monitoring
- [ ] VirtIO Network device
- [ ] Multi-core (SMP) support
- [ ] Simple filesystem
- [ ] Interactive games

---

**AR1 VCPU by APRK**
