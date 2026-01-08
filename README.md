# AR1 VCPU

**A Virtual CPU by APRK**

A lightweight ARM64 virtual CPU for Apple Silicon Macs, built using Apple's Hypervisor Framework. Features a live graphical dashboard, mouse support, and network capabilities.

![Platform](https://img.shields.io/badge/platform-macOS%20(Apple%20Silicon)-blue)
![License](https://img.shields.io/badge/license-MIT-green)
![Language](https://img.shields.io/badge/language-C%2B%2B20-orange)
![Graphics](https://img.shields.io/badge/graphics-SDL2%20%2B%20ImGui-red)

## Features

### Core
- **Native ARM64 Virtualization** - Uses Apple Hypervisor Framework (~0.2-1.0 MIPS)
- **GIC Interrupt Controller** - Timer interrupts, IRQ handling
- **VirtIO Block Device** - Disk read/write support

### Graphics & Input
- **SDL2 Graphics Window** - 320x200 framebuffer @ 60 FPS
- **Mouse Support** - Real-time cursor tracking with click detection
- **Keyboard Input** - Direct forwarding to VCPU UART

### Network
- **Network Device** - Ping and HTTP fetch capabilities
- **Kernel can ping** `8.8.8.8` and display results

### Debug Dashboard
- **ImGui System Monitor** - Apple-style minimalist UI
- **Live Disassembly** - ARM64 instruction decode via Capstone
- **Register View** - Real-time X0-X30 + PC monitoring

## Requirements

- macOS 11.0+ (Big Sur or later)
- Apple Silicon Mac (M1/M2/M3/M4/M5)
- Xcode Command Line Tools

## Dependencies

```bash
brew install sdl2 capstone
```

## Quick Start

```bash
# Clone
git clone https://github.com/APRK01/VCPU-AR1.git
cd VCPU-AR1

# Build Kernel
./build_os.sh

# Build VCPU
mkdir -p build && cd build
cmake .. && make

# Create Disk
echo "Hello from AR1!" > disk.img

# Run
./ar1_vcpu ../kernel.bin
```

## Controls

| Input | Action |
|-------|--------|
| **Mouse** | Move over VCPU screen to see cursor |
| **Click** | Cursor changes color (green = clicked) |
| **Keyboard** | Type into calculator prompt |
| **ESC** | Quit |

## Memory Map

| Address | Device |
|---------|--------|
| `0x09000000` | UART |
| `0x0A000000` | VirtIO Block |
| `0x0B000000` | Framebuffer |
| `0x0C000000` | Mouse |
| `0x0D000000` | Network |
| `0x40000000` | RAM (64MB) |

## Kernel Demo

On boot, the kernel:
1. Draws colored rectangles and gradient bar
2. **Pings 8.8.8.8** and displays result
3. Shows a mouse cursor that follows your pointer
4. Provides a calculator (type `5+3` and press Enter)

## Architecture

```
VCPU-AR1/
├── src/
│   ├── cpu/              # HVF wrapper, registers
│   ├── device/
│   │   ├── display.cpp   # SDL2 + ImGui + Mouse capture
│   │   ├── mouse.h       # Mouse MMIO device
│   │   ├── network.h     # Network (ping/fetch) device
│   │   └── framebuffer.cpp
│   └── main.cpp
├── os/
│   └── kernel.c          # Bare-metal kernel
└── include/
```

## Changelog

### v2.2.0 (Latest)
- ✅ **Mouse Support** - Cursor tracking and click detection
- ✅ **Network Device** - Ping and HTTP fetch
- ✅ Non-blocking UART for smooth mouse updates

### v2.1.0
- ImGui "System Monitor" dashboard
- Live ARM64 disassembly via Capstone
- Apple-style minimalist UI theme

### v2.0.0
- SDL2 graphics window
- Real-time performance stats
- Keyboard input forwarding

### v1.0.0
- Initial release
- HVF virtualization, VirtIO, GIC, Timer

## License

MIT License - see [LICENSE](LICENSE)

---

**AR1 VCPU by APRK**
