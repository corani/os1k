#!/bin/bash
set -xue

QEMU=qemu-system-riscv32
CC=clang-11
OBJCOPY=llvm-objcopy-11
LDFLAGS="-fuse-ld=lld-11"
CFLAGS="-std=c11 -O2 -g3 -Wall -Wextra --target=riscv32 -ffreestanding -nostdlib -mno-relax"

mkdir -p build

# Create disk image
(cd disk && tar cf ../build/disk.tar --format=ustar *.txt)

# Build userland
${CC} ${CFLAGS} ${LDFLAGS} -Wl,-Tuser/user.ld       \
                -Wl,-Map=build/shell.map            \
                -o build/shell.elf                  \
                user/*.c common.c 
${OBJCOPY} --set-section-flags .bss=alloc,contents -O binary build/shell.elf build/shell.bin
${OBJCOPY} -Ibinary -Oelf32-littleriscv build/shell.bin build/shell.bin.o

# Build the kernel
${CC} ${CFLAGS} ${LDFLAGS} -Wl,-Tkernel/kernel.ld   \
                -Wl,-Map=build/kernel.map           \
                -o build/kernel.elf                 \
                kernel/*.c common.c                 \
                build/shell.bin.o

# Download the OpenSBI firmware if it does not exist
if [ ! -f opensbi-riscv32-generic-fw_dynamic.bin ]; then
    curl -LO https://github.com/qemu/qemu/raw/v9.2.0/pc-bios/opensbi-riscv32-generic-fw_dynamic.bin
fi

# Run QEMU
${QEMU} -machine virt -bios default -nographic -serial mon:stdio --no-reboot        \
        -d unimp,guest_errors,int,cpu_reset -D qemu.log                             \
        -drive id=drive0,file=build/disk.tar,format=raw,if=none                     \
        -device virtio-blk-device,drive=drive0,bus=virtio-mmio-bus.0                \
        -kernel build/kernel.elf
