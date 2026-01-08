#!/bin/bash
clang -target aarch64-none-elf -c os/start.s -o os/start.o
clang -target aarch64-none-elf -ffreestanding -O3 -mgeneral-regs-only -c os/kernel.c -o os/kernel.o
ld.lld --oformat binary -Ttext 0x40000000 -e _start -o kernel.bin os/start.o os/kernel.o
echo "Build Done."
