#include "syscall.h"
#include "process.h"
#include "fs.h"

struct sbiret sbi_call(long arg0, long arg1, long arg2, long arg3, long arg4,
                       long arg5, long fid, long eid) {
    register long a0 __asm__("a0") = arg0;
    register long a1 __asm__("a1") = arg1;
    register long a2 __asm__("a2") = arg2;
    register long a3 __asm__("a3") = arg3;
    register long a4 __asm__("a4") = arg4;
    register long a5 __asm__("a5") = arg5;
    register long a6 __asm__("a6") = fid;
    register long a7 __asm__("a7") = eid;

    __asm__ __volatile__("ecall"
                         : "=r"(a0), "=r"(a1)                       // Output
                         : "r"(a0), "r"(a1), "r"(a2), "r"(a3),      // Input
                           "r"(a4), "r"(a5), "r"(a6), "r"(a7)
                         : "memory"                                 // Clobber
    );

    return (struct sbiret){.error = a0, .value = a1};
}

void putchar(char ch) {
    // SBI console putchar
    sbi_call(ch, 0, 0, 0, 0, 0, 0, 1);
}

int getchar(void) {
    // SBI console getchar
    struct sbiret ret = sbi_call(0, 0, 0, 0, 0, 0, 0, 2);
    return ret.error;
}

void handle_syscall(struct trap_frame *f) {
    switch (f->a3) {
        case SYS_PUTCHAR: {
            putchar(f->a0);
        } break;
        case SYS_GETCHAR: {
            for (;;) {
                long ch = getchar();
                if (ch >= 0) {
                    f->a0 = ch;
                    break;
                }

                yield();
            }
        } break;
        case SYS_EXIT: {
            printf("process %d exited\n", current_proc->pid);
            current_proc->state = PROC_EXITED;
            yield();
            PANIC("unreachable");
        } break;
        case SYS_READFILE:
        case SYS_WRITEFILE: {
            const char *filename = (const char *) f->a0;
            char *buf = (char *) f->a1;
            int len = f->a2;

            struct file *file = fs_lookup(filename);
            if (!file) {
                printf("file not found: %s\n", filename);
                f->a0 = -1;
                break;
            }

            if (len > (int) sizeof(file->data)) len = file->size;

            if (f->a3 == SYS_WRITEFILE) {
                memcpy(file->data, buf, len);
                file->size = len;
                fs_flush();
            } else {
                memcpy(buf, file->data, len);
            }

            f->a0 = len;
        } break;
        default:
            PANIC("unexpected syscall a3=%x\n", f->a3);
    }
}
