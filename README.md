# AR1 VCPU

**A Virtual CPU by APRK**

A lightweight ARM64 virtual CPU for Apple Silicon Macs, built using Apple's Hypervisor Framework. Features **Multi-Core SMP**, a **Mini GUI OS**, **Snake Game**, mouse support, and network capabilities.

![Platform](https://img.shields.io/badge/platform-macOS%20(Apple%20Silicon)-blue)
![License](https://img.shields.io/badge/license-MIT-green)
![Language](https://img.shields.io/badge/language-C%2B%2B20-orange)
![Cores](https://img.shields.io/badge/cores-2%20(SMP)-purple)

## Features

### Multi-Core SMP ⚡
- **2 vCPUs** running in parallel
- Core 0: Main GUI and app logic
- Core 1: Background tasks (visible counter in About window)

### Mini GUI OS 🖥️
- **Draggable Windows** with title bars and close buttons
- **Taskbar** with app switching
- **Mouse Cursor** that follows your pointer

### Snake Game 🐍
- Classic arcade game running on the VCPU
- WASD to move, eat food to grow
- SPACE to restart after game over

### Other Features
- SDL2 Graphics (320x200 @ 60fps)
- Network Device (ping, HTTP fetch)
- VirtIO Block Storage
- ImGui Debug Dashboard
- Live ARM64 Disassembly

## Requirements

- macOS 11.0+ (Apple Silicon)
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

# Run
./ar1_vcpu ../kernel.bin
```

## Controls

| Control | Action |
|---------|--------|
| **Mouse** | Move cursor, click windows |
| **Drag Title Bar** | Move windows |
| **Click Taskbar** | Switch apps (Calc, About, Snake) |
| **WASD** (in Snake) | Move snake |
| **SPACE** (in Snake) | Restart game |
| **Q** (in Snake) | Exit to desktop |
| **ESC** | Quit VCPU |

## Memory Map

| Address | Device |
|---------|--------|
| `0x09000000` | UART |
| `0x0A000000` | VirtIO Block |
| `0x0B000000` | Framebuffer |
| `0x0C000000` | Mouse |
| `0x0D000000` | Network |
| `0x40000000` | RAM (64MB) |

## Architecture

```
VCPU-AR1/
├── src/
│   ├── cpu/core.cpp      # HVF wrapper, multi-core support
│   ├── device/
│   │   ├── display.cpp   # SDL2 + ImGui + Mouse
│   │   ├── mouse.h       # Mouse MMIO
│   │   └── network.h     # Network device
│   └── main.cpp          # SMP thread management
├── os/
│   └── kernel.c          # GUI OS, Snake, multi-core
└── include/
```

## Changelog

### v2.3.0 (Latest) 🎉
- ✅ **Multi-Core SMP** - 2 vCPUs running in parallel!
- ✅ **Snake Game** - Classic arcade game
- ✅ **Mini GUI OS** - Draggable windows, taskbar, app switching
- ✅ Window title shows core count

### v2.2.0
- Mouse support with cursor tracking
- Network device (ping/fetch)
- Non-blocking UART

### v2.1.0
- ImGui debug dashboard
- Live ARM64 disassembly
- Apple-style UI theme

### v2.0.0
- SDL2 graphics window
- Keyboard input
- Real-time stats

## Screenshots

The VCPU window shows:
- **Left**: VCPU framebuffer with GUI OS
- **Right**: ImGui System Monitor with live disassembly

## License

MIT License - see [LICENSE](LICENSE)

---

**AR1 VCPU by APRK** - A true virtual computer! 🖥️
