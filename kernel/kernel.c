#include "kernel.h"
#include "memory.h"
#include "process.h"
#include "virtio.h"
#include "fs.h"
#include "syscall.h"

extern char __bss[], __bss_end[], __stack_top[];

// Application image.
extern char _binary_build_shell_bin_start[], _binary_build_shell_bin_size[];

void handle_trap(struct trap_frame *f) {
    uint32_t scause = READ_CSR(scause);
    uint32_t stval = READ_CSR(stval);
    uint32_t user_pc = READ_CSR(sepc);

    if (scause == SCAUSE_ECALL) {
        handle_syscall(f);
        // Advance user past ecall instruction.
        user_pc += 4;
    } else {
        PANIC("unexpected trap scause=%x, stval=%x, sepc=%x\n", scause, stval, user_pc);
    }

    WRITE_CSR(sepc, user_pc);
}

__attribute__((naked))
__attribute__((aligned(4)))
void kernel_entry(void) {
    __asm__ __volatile__(
        // Read kernel stack pointer of the running process from sscratch (written in yield).
        "csrrw sp, sscratch, sp\n"
        "addi sp, sp, -4*31\n"  // Reserve space for registers
        "sw  ra, 4* 0(sp)\n"
        "sw  gp, 4* 1(sp)\n"
        "sw  tp, 4* 2(sp)\n"
        "sw  t0, 4* 3(sp)\n"
        "sw  t1, 4* 4(sp)\n"
        "sw  t2, 4* 5(sp)\n"
        "sw  t3, 4* 6(sp)\n"
        "sw  t4, 4* 7(sp)\n"
        "sw  t5, 4* 8(sp)\n"
        "sw  t6, 4* 9(sp)\n"
        "sw  a0, 4*10(sp)\n"
        "sw  a1, 4*11(sp)\n"
        "sw  a2, 4*12(sp)\n"
        "sw  a3, 4*13(sp)\n"
        "sw  a4, 4*14(sp)\n"
        "sw  a5, 4*15(sp)\n"
        "sw  a6, 4*16(sp)\n"
        "sw  a7, 4*17(sp)\n"
        "sw  s0, 4*18(sp)\n"
        "sw  s1, 4*19(sp)\n"
        "sw  s2, 4*20(sp)\n"
        "sw  s3, 4*21(sp)\n"
        "sw  s4, 4*22(sp)\n"
        "sw  s5, 4*23(sp)\n"
        "sw  s6, 4*24(sp)\n"
        "sw  s7, 4*25(sp)\n"
        "sw  s8, 4*26(sp)\n"
        "sw  s9, 4*27(sp)\n"
        "sw s10, 4*28(sp)\n"
        "sw s11, 4*29(sp)\n"

        // Retrieve and save the sp at the time of execution.
        "csrr a0, sscratch\n"
        "sw  a0, 4*30(sp)\n"

        // Reset the kernel stack.
        "addi a0, sp, 4 * 31\n"
        "csrw sscratch, a0\n"

        // Trap
        "mv a0, sp\n"
        "call handle_trap\n"

        "lw  ra, 4* 0(sp)\n"
        "lw  gp, 4* 1(sp)\n"
        "lw  tp, 4* 2(sp)\n"
        "lw  t0, 4* 3(sp)\n"
        "lw  t1, 4* 4(sp)\n"
        "lw  t2, 4* 5(sp)\n"
        "lw  t3, 4* 6(sp)\n"
        "lw  t4, 4* 7(sp)\n"
        "lw  t5, 4* 8(sp)\n"
        "lw  t6, 4* 9(sp)\n"
        "lw  a0, 4*10(sp)\n"
        "lw  a1, 4*11(sp)\n"
        "lw  a2, 4*12(sp)\n"
        "lw  a3, 4*13(sp)\n"
        "lw  a4, 4*14(sp)\n"
        "lw  a5, 4*15(sp)\n"
        "lw  a6, 4*16(sp)\n"
        "lw  a7, 4*17(sp)\n"
        "lw  s0, 4*18(sp)\n"
        "lw  s1, 4*19(sp)\n"
        "lw  s2, 4*20(sp)\n"
        "lw  s3, 4*21(sp)\n"
        "lw  s4, 4*22(sp)\n"
        "lw  s5, 4*23(sp)\n"
        "lw  s6, 4*24(sp)\n"
        "lw  s7, 4*25(sp)\n"
        "lw  s8, 4*26(sp)\n"
        "lw  s9, 4*27(sp)\n"
        "lw s10, 4*28(sp)\n"
        "lw s11, 4*29(sp)\n"
        "lw  sp, 4*30(sp)\n"

        "sret\n"
    );
}

void kernel_main(void) {
    memset(__bss, 0, (size_t) __bss_end - (size_t) __bss);
    WRITE_CSR(stvec, (uint32_t) kernel_entry);

    virtio_blk_init();
    fs_init();

    idle_proc = create_process(NULL, 0);
    idle_proc->pid = -1; // idle
    current_proc = idle_proc;

    create_process(_binary_build_shell_bin_start, (size_t) _binary_build_shell_bin_size);

    yield();

    PANIC("switched to idle process");
}

__attribute__((section(".text.boot")))
__attribute__((naked))
void boot(void) {
    __asm__ __volatile__(
        "mv sp, %[stack_top]\n"         // Setup stack pointer
        "j kernel_main\n"               // Jump to kernel main
        :                               // Output
        : [stack_top] "r" (__stack_top) // Input
    );
}
