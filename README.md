# AR1 VCPU

**A Virtual CPU by APRK**

A lightweight ARM64 virtual CPU for Apple Silicon Macs, built using Apple's Hypervisor Framework. Featuring a **live graphical dashboard**, real-time disassembly, and SDL2 graphics.

![Platform](https://img.shields.io/badge/platform-macOS%20(Apple%20Silicon)-blue)
![License](https://img.shields.io/badge/license-MIT-green)
![Language](https://img.shields.io/badge/language-C%2B%2B20-orange)
![Graphics](https://img.shields.io/badge/graphics-SDL2%20%2B%20ImGui-red)

## Features

- **Native ARM64 Virtualization** - Uses Apple Framework for near-native performance (~0.2-0.3 MIPS).
- **Pro Dashboard** - Live system monitor overlay using Dear ImGui:
  - **Live Disassembly** using Capstone engine (watch instructions execute!)
  - **Register View** (Real-time X0-X30 monitoring)
  - **VCPU Status**
- **SDL2 Graphics Window** - 320x200 framebuffer with 3x scaling (960x600 window).
- **Keyboard Input** - Forwarded directly to the VCPU over UART.
- **VirtIO Storage** - Block device support.
- **Bare-Metal Kernel** - Includes a demo with graphics and calculator.

## Screen & Dashboard

The emulator opens a single window with two sections:
1.  **VCPU Display**: Shows the 320x200 framebuffer output (colored graphics).
2.  **System Monitor**: A "Glass Cockpit" showing what the CPU is actually doing.

## Requirements

- macOS 11.0+ (Big Sur or later)
- Apple Silicon Mac (M1/M2/M3/M4/M5)
- Xcode Command Line Tools
- Homebrew

## Dependencies

```bash
brew install sdl2 capstone
```

## Quick Start

### Build

```bash
# Clone
git clone https://github.com/APRK01/VCPU-AR1.git
cd VCPU-AR1

# Build Kernel
./build_os.sh

# Build Emulator
mkdir -p build && cd build
cmake ..
make

# Create Disk
echo "Hello from AR1!" > disk.img
```

### Run

```bash
./build/ar1_vcpu kernel.bin
```

## Controls

- **Keyboard**: Type into the VCPU UART console.
- **ESC**: Quit the emulator.

## Architecture

```
VCPU-AR1/
├── src/
│   ├── cpu/              # Core logic & HVF wrapper
│   ├── device/
│   │   ├── display.cpp   # SDL2 + ImGui Dashboard Implementation
│   │   └── framebuffer.cpp
│   ├── vendor/           # ImGui source code
│   └── main.cpp          # Entry point
├── os/                   # Bare-metal kernel code
└── build_os.sh
```

## Performance

The dashboard shows real-time stats in the window title:
```
AR1 VCPU | 0.95 MIPS
```

## License

MIT License - see [LICENSE](LICENSE) file for details.

---

**AR1 VCPU by APRK**
