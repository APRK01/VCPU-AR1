# AR1 VCPU

**A Virtual CPU by APRK**

A lightweight ARM64 virtual CPU for Apple Silicon Macs, built using Apple's Hypervisor Framework. Features real-time graphics, mouse input, keyboard demo, and a live debug dashboard.

![Platform](https://img.shields.io/badge/platform-macOS%20(Apple%20Silicon)-blue)
![License](https://img.shields.io/badge/license-MIT-green)
![Language](https://img.shields.io/badge/language-C%2B%2B20-orange)

## Features

### Graphics & Input
- **SDL2 Graphics** - 320x200 framebuffer @ 60fps
- **Mouse Support** - Cursor tracking with click detection
- **Keyboard Input** - Real-time key capture and display

### Devices
- **UART** - Serial I/O for debugging
- **Framebuffer** - Pixel-level graphics control
- **Mouse** - Position and button state
- **Network** - Ping and HTTP fetch (experimental)
- **VirtIO Block** - Disk storage

### Debug Dashboard
- **ImGui System Monitor** - Apple-style UI
- **Live Registers** - X0-X7, PC, SP
- **ARM64 Disassembly** - Via Capstone

## Requirements

- macOS 11.0+ (Apple Silicon M1/M2/M3/M4)
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

## Current Demo

The keyboard demo shows:
- **7-Segment Counter** - Shows how many characters typed
- **Visual Blocks** - Last 10 typed characters
- **Blinking Cursor** - Shows input position
- **Status Bar** - System status

### Controls
| Key | Action |
|-----|--------|
| Any key | Type character |
| Backspace | Delete last |
| ESC | Clear all |

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
│   ├── cpu/          # HVF wrapper, registers
│   ├── device/       # Display, mouse, network
│   └── soc/          # Bus, RAM, GIC
├── os/
│   └── kernel.c      # Bare-metal kernel
└── include/
```

## Changelog

### v2.4.0 (Current)
- ✅ Keyboard demo with visual feedback
- ✅ 7-segment digit display
- ✅ Character counter and blocks

### v2.3.0
- Mouse cursor support
- System Monitor improvements
- Color format fixes

### v2.2.0
- Network device (ping/fetch)
- Non-blocking UART

### v2.1.0
- ImGui debug dashboard
- Live ARM64 disassembly

## License

MIT License

---

**AR1 VCPU by APRK** 🖥️
