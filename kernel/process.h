#pragma once 

#include "../common.h"

#define PROCS_MAX 8         // Maximum number of processes
#define PROC_UNUSED   0     // Unused process control structure
#define PROC_RUNNABLE 1     // Runnable process
#define PROC_EXITED   2     // Exited process

struct process {
    int pid;                // Process ID
    int state;              // Process state
    vaddr_t sp;             // Stack pointer
    uint32_t *page_table;   // Page table
    uint8_t stack[8192];    // Kernel Stack
};

extern struct process *current_proc;
extern struct process *idle_proc;

struct process *create_process(const void *image, size_t image_size);
void yield(void);

// The base virtual address of an application image. This needs to match the
// starting address defined in `user.ld`.
#define USER_BASE 0x1000000
#define SSTATUS_SPIE (1 << 5)
#define SSTATUS_SUM  (1 << 18)

#define SATP_SV32 (1u << 31)

