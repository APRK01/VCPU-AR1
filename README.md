# AR1 VCPU

**A Virtual CPU by APRK**

A lightweight ARM64 virtual CPU for Apple Silicon Macs, built using Apple's Hypervisor Framework with **SDL2 graphics**.

![Platform](https://img.shields.io/badge/platform-macOS%20(Apple%20Silicon)-blue)
![License](https://img.shields.io/badge/license-MIT-green)
![Language](https://img.shields.io/badge/language-C%2B%2B20-orange)
![Graphics](https://img.shields.io/badge/graphics-SDL2-red)

## Features

- **Native ARM64 Virtualization** - Uses Apple Hypervisor Framework for near-native performance
- **SDL2 Graphics Window** - Real-time 320x200 framebuffer display at 60 FPS
- **Keyboard Input** - Keys are forwarded directly to the VCPU
- **Live Performance Stats** - Real-time MIPS, instruction count, and FPS in window title
- **Interrupt Support** - GIC (Generic Interrupt Controller) with timer interrupts
- **VirtIO Storage** - VirtIO block device for disk I/O
- **UART Console** - Serial I/O for text-based interaction
- **Bare-Metal Kernel** - Includes a demo kernel with graphics, calculator, and more

## Requirements

- macOS 11.0+ (Big Sur or later)
- Apple Silicon Mac (M1/M2/M3/M4/M5)
- Xcode Command Line Tools
- CMake 3.16+
- SDL2 (`brew install sdl2`)

## Quick Start

### Build

```bash
# Clone the repository
git clone https://github.com/APRK01/VCPU-AR1.git
cd VCPU-AR1

# Install SDL2 (if not already installed)
brew install sdl2

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

A graphics window will open showing the VCPU output. The window title shows real-time performance stats.

**Controls:**
- **Keyboard**: Keys are sent directly to the VCPU
- **ESC**: Quit the emulator

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
│   │   ├── framebuffer.cpp   # 320x200 Framebuffer
│   │   └── display.cpp       # SDL2 Graphics Window
│   └── main.cpp          # Entry point & main loop
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

## Demo Kernel

The included kernel demonstrates:
- **Live Graphics** - Draws colorful rectangles and gradient bars
- **Timer Interrupts** - Periodic timer ticks
- **VirtIO Disk Read** - Reads sector 0 from disk.img
- **Calculator** - Enter expressions like `5+3`, `10*2`, etc.

## Performance

The VCPU achieves approximately **0.2-0.3 MIPS** on Apple Silicon with:
- **~60 FPS** display refresh rate
- Hardware-accelerated virtualization via Apple HVF

Window title shows real-time stats:
```
AR1 VCPU | 0.23 MIPS | 1606758 instr | 7s | FPS: 57
```

## Dependencies

| Library | Purpose |
|---------|---------|
| **SDL2** | Graphics window, keyboard input |
| **Hypervisor.framework** | ARM64 virtualization |

Install SDL2:
```bash
brew install sdl2
```

## License

MIT License - see [LICENSE](LICENSE) file for details.

## Acknowledgments

- Apple Hypervisor Framework documentation
- VirtIO specification
- ARM Architecture Reference Manual
- SDL2 library

## Roadmap

- [x] SDL2 Graphics Window
- [x] Real-time Performance Monitoring
- [x] Keyboard Input
- [ ] VirtIO Network device
- [ ] Multi-core (SMP) support
- [ ] Audio support
- [ ] Interactive games

---

**AR1 VCPU by APRK**
