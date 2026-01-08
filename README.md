# AR1 VCPU

A lightweight ARM64 virtual CPU emulator for Apple Silicon Macs, built using Apple's Hypervisor Framework.

![Platform](https://img.shields.io/badge/platform-macOS%20(Apple%20Silicon)-blue)
![License](https://img.shields.io/badge/license-MIT-green)
![Language](https://img.shields.io/badge/language-C%2B%2B17-orange)

## ✨ Features

- **Native ARM64 Virtualization** - Uses Apple Hypervisor Framework for near-native performance
- **Interrupt Support** - GIC (Generic Interrupt Controller) with timer interrupts
- **VirtIO Storage** - VirtIO block device for disk I/O
- **UART Console** - Serial I/O for text-based interaction
- **Interactive Shell** - Debug and control the VCPU at runtime
- **Bare-Metal Kernel** - Includes a simple demo kernel with a calculator

## 📋 Requirements

- macOS 11.0+ (Big Sur or later)
- Apple Silicon Mac (M1/M2/M3)
- Xcode Command Line Tools
- CMake 3.16+

## 🚀 Quick Start

### Build

```bash
# Clone the repository
git clone https://github.com/yourusername/ar1-vcpu.git
cd ar1-vcpu

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
```

## 🏗️ Architecture

```
ar1-vcpu/
├── src/
│   ├── cpu/          # VCPU core (HVF integration)
│   ├── soc/          # System-on-Chip components
│   │   ├── bus.cpp   # Memory bus & MMIO routing
│   │   ├── gic.cpp   # Interrupt controller
│   │   ├── ram.cpp   # RAM management
│   │   └── uart.cpp  # Serial I/O
│   ├── device/       # Virtual devices
│   │   └── virtio_blk.cpp  # VirtIO block storage
│   └── main.cpp      # Entry point & shell
├── os/               # Bare-metal kernel
│   ├── start.s       # Boot assembly
│   ├── kernel.c      # Kernel C code
│   └── linker.ld     # Linker script
├── include/          # Headers
└── build_os.sh       # Kernel build script
```

## 💡 Memory Map

| Address | Size | Description |
|---------|------|-------------|
| `0x09000000` | 4KB | UART (PL011-compatible) |
| `0x0A000000` | 512B | VirtIO Block Device |
| `0x0F000000` | 64KB | GIC Distributor |
| `0x0F100000` | 8KB | GIC CPU Interface |
| `0x40000000` | 64MB | RAM |
| `0x50000000` | 16MB | VRAM (reserved) |

## 🎮 Shell Commands

| Command | Description |
|---------|-------------|
| `run` | Resume VCPU execution |
| `pause` | Pause VCPU execution |
| `step` | Single-step one instruction |
| `regs` | Display CPU registers |
| `mem <addr>` | Dump memory at address |
| `help` | Show available commands |
| `quit` | Exit emulator |

## 🔧 Demo Kernel

The included kernel demonstrates:
- Timer interrupt handling (prints `!` on each tick)
- VirtIO disk read (reads sector 0 and prints content)
- Simple calculator (enter expressions like `2+3`)

## 📝 License

MIT License - see [LICENSE](LICENSE) file for details.

## 🙏 Acknowledgments

- Apple Hypervisor Framework documentation
- VirtIO specification
- ARM Architecture Reference Manual

## 🗺️ Roadmap

- [ ] Framebuffer/Graphics support
- [ ] VirtIO Network device
- [ ] Multi-core (SMP) support
- [ ] Simple filesystem
- [ ] More kernel demos

---

**Built with ❤️ for Apple Silicon**
