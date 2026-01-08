#pragma once
#include "../soc/bus.h"
#include "../types.h"
#include "registers.h"
#include <Hypervisor/Hypervisor.h>
#include <memory>

namespace ar1 {

class ICache;
class BranchPredictor;

class Core {
public:
  Core(std::shared_ptr<Bus> system_bus, u32 id = 0);
  ~Core();

  void reset(u64 start_addr);
  // Run for specific instruction count
  void run(size_t count);

  // Debugging
  void dump_regs();

  // Register Access
  u64 get_reg(u32 reg) { return regs.x[reg]; }
  u64 get_pc() { return regs.pc; }
  void clock();
  void update_timer();

  void set_pc(u64 addr);
  void set_reg(u32 idx, u64 val);
  u64 get_pc() const;

  Registers regs;
  u32 core_id;
  bool halted;

  std::unique_ptr<ICache> icache;
  std::unique_ptr<BranchPredictor> predictor;

  // Apple Hypervisor Framework handle
  hv_vcpu_t vcpu;
  hv_vcpu_exit_t *vcpu_exit;
  bool hvf_initialized;

private:
  std::shared_ptr<Bus> bus;

  void handle_exit();
  u64 handle_mmio_read(u64 addr, u32 size);
  void handle_mmio_write(u64 addr, u32 size, u64 value);
  void sync_regs_to_hvf();
  void sync_regs_from_hvf();
};

} // namespace ar1