#include "process.h"
#include "memory.h"
#include "virtio.h"

extern char __free_ram[], __free_ram_end[];
extern char __kernel_base[];

struct process procs[PROCS_MAX];
struct process *current_proc;
struct process *idle_proc;

__attribute__((naked))
void switch_context(uint32_t *prev_sp, uint32_t *next_sp) {
    // NOTE(daniel): a0 = prev_sp, a1 = next_sp
    __asm__ __volatile__(
        "addi sp, sp, -13 * 4\n"    // Reserve space for registers
        "sw  ra,  0 * 4(sp)\n"      // Save callee-saved registers only
        "sw  s0,  1 * 4(sp)\n"
        "sw  s1,  2 * 4(sp)\n"
        "sw  s2,  3 * 4(sp)\n"
        "sw  s3,  4 * 4(sp)\n"
        "sw  s4,  5 * 4(sp)\n"
        "sw  s5,  6 * 4(sp)\n"
        "sw  s6,  7 * 4(sp)\n"
        "sw  s7,  8 * 4(sp)\n"
        "sw  s8,  9 * 4(sp)\n"
        "sw  s9, 10 * 4(sp)\n"
        "sw s10, 11 * 4(sp)\n"
        "sw s11, 12 * 4(sp)\n"
        "sw sp, (a0)\n"             // *prev_sp = sp
        "lw sp, (a1)\n"             // sp = *next_sp
        "lw  ra,  0 * 4(sp)\n"      // Restore callee-saved registers only
        "lw  s0,  1 * 4(sp)\n"
        "lw  s1,  2 * 4(sp)\n"
        "lw  s2,  3 * 4(sp)\n"
        "lw  s3,  4 * 4(sp)\n"
        "lw  s4,  5 * 4(sp)\n"
        "lw  s5,  6 * 4(sp)\n"
        "lw  s6,  7 * 4(sp)\n"
        "lw  s7,  8 * 4(sp)\n"
        "lw  s8,  9 * 4(sp)\n"
        "lw  s9, 10 * 4(sp)\n"
        "lw s10, 11 * 4(sp)\n"
        "lw s11, 12 * 4(sp)\n"
        "addi sp, sp, 13 * 4\n"     // We've popped 13 registers from the stack
        "ret\n"
    );
}

__attribute__((naked))
void user_entry(void) {
    __asm__ __volatile__(
        "csrw sepc, %[sepc]         \n"
        "csrw sstatus, %[sstatus]   \n"
        "sret                       \n"
        :
        : [sepc]    "r" (USER_BASE),
          [sstatus] "r" (SSTATUS_SPIE | SSTATUS_SUM)
    );
}

struct process *create_process(const void *image, size_t image_size) {
    struct process *proc = NULL;
    int i;
    for (i = 0; i < PROCS_MAX; ++i) {
        if (procs[i].state == PROC_UNUSED) {
            proc = &procs[i];
            break;
        }
    }

    if (!proc) {
        PANIC("no free process slots");
    }

    // Stack callee-saved registers.
    uint32_t *sp = (uint32_t *) &proc->stack[sizeof(proc->stack)];
    *--sp = 0;              // s11
    *--sp = 0;              // s10
    *--sp = 0;              // s9
    *--sp = 0;              // s8
    *--sp = 0;              // s7
    *--sp = 0;              // s6
    *--sp = 0;              // s5
    *--sp = 0;              // s4
    *--sp = 0;              // s3
    *--sp = 0;              // s2
    *--sp = 0;              // s1
    *--sp = 0;              // s0
    *--sp = (uint32_t) user_entry;  // ra
   
    uint32_t *page_table = (uint32_t *) alloc_pages(1);

    // Map kernel pages.
    for (paddr_t paddr = (paddr_t) __kernel_base; paddr < (paddr_t) __free_ram_end; paddr += PAGE_SIZE) {
        map_page(page_table, paddr, paddr, PAGE_R | PAGE_W | PAGE_X);
    }

    // Map virtio block device.
    map_page(page_table, VIRTIO_BLK_PADDR, VIRTIO_BLK_PADDR, PAGE_R | PAGE_W);

    // Map user pages.
    for (uint32_t off = 0; off < image_size; off += PAGE_SIZE) {
        paddr_t page = alloc_pages(1);

        // Copy at most one page size.
        size_t remaining = image_size - off;
        size_t copy_size = PAGE_SIZE <= remaining ? PAGE_SIZE : remaining;

        memcpy((void *) page, image + off, copy_size);
        map_page(page_table, USER_BASE + off, page, PAGE_U | PAGE_R | PAGE_W | PAGE_X);
    }

    // Initialize fields.
    proc->pid = i + 1;
    proc->state = PROC_RUNNABLE;
    proc->sp = (uint32_t) sp;
    proc->page_table = page_table;

    return proc;
}

void yield(void) {
    // Search for runnable process
    struct process *next = idle_proc;
    for (int i = 0; i < PROCS_MAX; i++) {
        struct process *proc = &procs[(current_proc->pid + i) % PROCS_MAX];
        if (proc->state == PROC_RUNNABLE && proc->pid > 0) {
            next = proc;
            break;
        }
    }

    // If there's no runnable process other than the current one, return and continue
    if (next == current_proc) return;

    __asm__ __volatile__(
        // Switch process' page table
        "sfence.vma\n"
        "csrw satp, %[satp]\n"
        "sfence.vma\n"
        // Write kernel stack pointer of running process to sscratch (read in kernel_entry)
        "csrw sscratch, %[sscratch]\n"
        : 
        : [satp] "r" (SATP_SV32 | ((uint32_t) next->page_table / PAGE_SIZE)),
          [sscratch] "r" ((uint32_t) &next->stack[sizeof(next->stack)])
    );

    // Context switch 
    struct process *prev = current_proc;
    current_proc = next;
    switch_context(&prev->sp, &next->sp);
}

