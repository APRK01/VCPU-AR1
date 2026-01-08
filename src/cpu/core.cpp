#include "cpu/core.h"
#include "cpu/cache.h"
#include "cpu/predictor.h"
#include <cstring>
#include <deque>
#include <iostream>
#include <mutex>
#include <pthread.h>

namespace ar1 {

// Global UART Queue
static std::deque<char> uart_rx_queue;
static std::mutex uart_mutex;

void uart_push(char c) {
  std::lock_guard<std::mutex> lock(uart_mutex);
  uart_rx_queue.push_back(c);
}

Core::Core(std::shared_ptr<Bus> system_bus, u32 id)
    : core_id(id), halted(false), hvf_initialized(false), bus(system_bus) {

  icache = std::make_unique<ICache>();
  predictor = std::make_unique<BranchPredictor>();

  static std::once_flag vm_init_flag;

  std::call_once(vm_init_flag, [&]() {
    hv_return_t ret = hv_vm_create(NULL);
    if (ret != HV_SUCCESS) {
      std::cerr << "[AR1] Failed to create HVF VM: " << ret << std::endl;
      std::cerr << "[AR1] Check entitlements!" << std::endl;
      return;
    }

    ret = hv_vm_map(bus->ram->get_raw_ptr(), RAM_BASE, RAM_SIZE,
                    HV_MEMORY_READ | HV_MEMORY_WRITE | HV_MEMORY_EXEC);
    if (ret != HV_SUCCESS)
      std::cerr << "[AR1] Failed to map RAM: " << ret << std::endl;

    ret = hv_vm_map(bus->vram->get_raw_ptr(), VRAM_BASE, VRAM_SIZE,
                    HV_MEMORY_READ | HV_MEMORY_WRITE);
    if (ret != HV_SUCCESS)
      std::cerr << "[AR1] Failed to map VRAM: " << ret << std::endl;

    std::cout << "[AR1] HVF VM created successfully" << std::endl;
  });

  hv_return_t ret = hv_vcpu_create(&vcpu, &vcpu_exit, NULL);
  if (ret != HV_SUCCESS) {
    std::cerr << "[AR1] Failed to create vCPU: " << ret << std::endl;
    return;
  }

  hvf_initialized = true;
  std::cout << "[AR1] Core " << id << " initialized." << std::endl;
}

Core::~Core() {
  if (hvf_initialized) {
    hv_vcpu_destroy(vcpu);
  }
}

void Core::reset(u64 start_addr) {
  if (!hvf_initialized)
    return;

  regs.pc = start_addr;
  halted = false;

  for (int i = 0; i < 31; i++) {
    hv_vcpu_set_reg(vcpu, static_cast<hv_reg_t>(HV_REG_X0 + i), 0);
  }

  hv_vcpu_set_reg(vcpu, HV_REG_PC, start_addr);

  u64 sp = RAM_BASE + RAM_SIZE - 0x10000 - (core_id * 0x10000);
  hv_vcpu_set_sys_reg(vcpu, HV_SYS_REG_SP_EL0, sp);
  hv_vcpu_set_sys_reg(vcpu, HV_SYS_REG_SP_EL1, sp);

  hv_vcpu_set_sys_reg(vcpu, HV_SYS_REG_MPIDR_EL1, core_id);

  u64 dtb_addr = RAM_BASE + 0x08000000;
  hv_vcpu_set_reg(vcpu, HV_REG_X0, dtb_addr);

  hv_vcpu_set_reg(vcpu, HV_REG_CPSR, 0x3c5);
  hv_vcpu_set_sys_reg(vcpu, HV_SYS_REG_CPACR_EL1, 0x300000); // Enable FP/SIMD

  std::cout << "[AR1] Core " << core_id << " Reset." << std::endl;
}

void Core::set_pc(u64 addr) {
  regs.pc = addr;
  if (hvf_initialized) {
    hv_vcpu_set_reg(vcpu, HV_REG_PC, addr);
  }
}

void Core::set_reg(u32 idx, u64 val) {
  if (idx < 31) {
    regs.x[idx] = val;
    if (hvf_initialized) {
      hv_vcpu_set_reg(vcpu, static_cast<hv_reg_t>(HV_REG_X0 + idx), val);
    }
  }
}

u64 Core::get_pc() const { return regs.pc; }

void Core::dump_regs() {
  std::cout << "--- Core " << core_id << " Registers ---" << std::endl;
  for (int i = 0; i < 31; i++) {
    std::cout << "X" << std::dec << i << ": 0x" << std::hex << regs.x[i]
              << "  ";
    if ((i + 1) % 4 == 0)
      std::cout << std::endl;
  }
  std::cout << "\nPC: 0x" << std::hex << regs.pc << std::dec << std::endl;
}

void Core::sync_regs_to_hvf() {
  if (!hvf_initialized)
    return;
  for (int i = 0; i < 31; i++) {
    hv_vcpu_set_reg(vcpu, static_cast<hv_reg_t>(HV_REG_X0 + i), regs.x[i]);
  }
  hv_vcpu_set_reg(vcpu, HV_REG_PC, regs.pc);
}

void Core::sync_regs_from_hvf() {
  if (!hvf_initialized)
    return;
  for (int i = 0; i < 31; i++) {
    u64 val;
    hv_vcpu_get_reg(vcpu, static_cast<hv_reg_t>(HV_REG_X0 + i), &val);
    regs.x[i] = val;
  }
  u64 pc;
  hv_vcpu_get_reg(vcpu, HV_REG_PC, &pc);
  regs.pc = pc;
}

u64 Core::handle_mmio_read(u64 addr, u32 size) {
  (void)size;
  u64 value = 0;

  if (addr >= UART_BASE && addr < UART_BASE + 0x1000) {
    if (addr == UART_BASE) {
      std::lock_guard<std::mutex> lock(uart_mutex);
      if (!uart_rx_queue.empty()) {
        value = uart_rx_queue.front();
        uart_rx_queue.pop_front();
      } else {
        value = 0;
      }
    } else if (addr == UART_BASE + 0x18) {
      value = 0x80;
      std::lock_guard<std::mutex> lock(uart_mutex);
      if (uart_rx_queue.empty()) {
        value |= 0x10;
      }
    }
  } else {
    // Forward to Bus for other devices (GIC, etc)
    // Note: Bus::read/write normally routes TO Core, but here Core is routing
    // FROM Exit. Core has a Bus pointer, so we can use it. Since Bus::readN
    // checks ranges, we can just defer to it if we want full IO. However,
    // Core::handle_mmio_read is explicitly called for MMIO exits.
    if (addr >= GIC_DIST_BASE && addr < GIC_CPU_BASE + 0x2000) {
      if (size == 4)
        value = bus->read32(addr);
    } else if (addr >= 0x0A000000 && addr < 0x0A000200) {
      if (size == 4)
        value = bus->read32(addr);
    } else if (addr >= 0x0B000000 && addr < 0x0B000100) {
      if (size == 4)
        value = bus->read32(addr);
    }
  }
  return value;
}

void Core::handle_mmio_write(u64 addr, u32 size, u64 value) {
  (void)size;

  if (addr >= UART_BASE && addr < UART_BASE + 0x1000) {
    if (addr == UART_BASE) {
      char c = static_cast<char>(value & 0xFF);
      std::cout << c << std::flush;
    }
  } else {
    if (addr >= GIC_DIST_BASE && addr < GIC_CPU_BASE + 0x2000) {
      if (size == 4)
        bus->write32(addr, (u32)value);
    } else if (addr >= 0x0A000000 && addr < 0x0A000200) {
      if (size == 4)
        bus->write32(addr, (u32)value);
    } else if (addr >= 0x0B000000 && addr < 0x0B000100) {
      if (size == 4)
        bus->write32(addr, (u32)value);
    }
  }
}

void Core::handle_exit() {
  if (!hvf_initialized)
    return;

  switch (vcpu_exit->reason) {
  case HV_EXIT_REASON_EXCEPTION: {
    u64 esr = vcpu_exit->exception.syndrome;
    u32 ec = static_cast<u32>((esr >> 26) & 0x3F);

    static u64 exc_count = 0;
    exc_count++;
    if (exc_count <= 10) {
      u64 pc_dbg;
      hv_vcpu_get_reg(vcpu, HV_REG_PC, &pc_dbg);
      std::cout << "[AR1] Exception EC=0x" << std::hex << ec << " ESR=0x" << esr
                << " PC=0x" << pc_dbg << std::dec << std::endl;
    }

    if (ec == 0x00 || ec == 0x01) {
      u64 pc;
      hv_vcpu_get_reg(vcpu, HV_REG_PC, &pc);
      hv_vcpu_set_reg(vcpu, HV_REG_PC, pc + 4);
    } else if (ec == 0x16 || ec == 0x17) { // HVC/SMC
      u64 func_id;
      hv_vcpu_get_reg(vcpu, HV_REG_X0, &func_id);
      hv_vcpu_set_reg(vcpu, HV_REG_X0, 0);
      u64 pc;
      hv_vcpu_get_reg(vcpu, HV_REG_PC, &pc);
      hv_vcpu_set_reg(vcpu, HV_REG_PC, pc + 4);
    } else if (ec == 0x24) { // Data Abort
      u64 pa = vcpu_exit->exception.physical_address;
      bool isv = (esr >> 24) & 1;
      if (isv) {
        int sas = (esr >> 22) & 3;
        int len = 1 << sas;
        bool is_write = (esr >> 6) & 1;
        int rt = (esr >> 16) & 0x1F;
        if (is_write) {
          u64 val = 0;
          if (rt != 31) {
            hv_vcpu_get_reg(vcpu, (hv_reg_t)(HV_REG_X0 + rt), &val);
          }
          if (len == 1)
            val &= 0xFF;
          else if (len == 2)
            val &= 0xFFFF;
          else if (len == 4)
            val &= 0xFFFFFFFF;
          handle_mmio_write(pa, len, val);
        } else {
          u64 val = handle_mmio_read(pa, len);
          if (rt != 31) {
            hv_vcpu_set_reg(vcpu, (hv_reg_t)(HV_REG_X0 + rt), val);
          }
        }
      }
      u64 pc;
      hv_vcpu_get_reg(vcpu, HV_REG_PC, &pc);
      hv_vcpu_set_reg(vcpu, HV_REG_PC, pc + 4);
    }
    break;
  }
  case HV_EXIT_REASON_VTIMER_ACTIVATED:
    // std::cerr << "[AR1] VTIMER Exit!" << std::endl;
    bus->gic->set_irq(27, true);
    break;
  case HV_EXIT_REASON_CANCELED:
    halted = true;
    break;
  default:
    std::cerr << "[AR1] Unknown exit: " << vcpu_exit->reason << std::endl;
    halted = true;
    break;
  }
}

void Core::update_timer() {}

void Core::clock() {
  if (halted)
    return;
  run(100);
}

void Core::run(size_t count) {
  if (halted || !hvf_initialized)
    return;

  if (__builtin_available(macOS 11.0, *)) {
    pthread_jit_write_protect_np(1); // Enable Exec
  }

  for (size_t i = 0; i < count; i++) {
    if (halted)
      break;
    hv_return_t ret = hv_vcpu_run(vcpu);
    if (ret != HV_SUCCESS) {
      std::cerr << "[AR1] vCPU run failed: " << ret << std::endl;
      halted = true;
      return;
    }
    handle_exit();
    instructions_executed++;

    if (bus->gic->is_irq_pending()) {
      hv_vcpu_set_pending_interrupt(vcpu, HV_INTERRUPT_TYPE_IRQ, true);
    } else {
      hv_vcpu_set_pending_interrupt(vcpu, HV_INTERRUPT_TYPE_IRQ, false);
    }
  }
  sync_regs_from_hvf();
}

} // namespace ar1
