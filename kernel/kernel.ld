/* entrypoint defined in kernel.c */
ENTRY(boot)

SECTIONS {
    /* base address */
    . = 0x80200000;
    __kernel_base = .;

    /* program code */
    .text : {
        KEEP(*(.text.boot));
        *(.text .text.*);
    }

    /* read only data */
    .rodata : ALIGN(4) {
        *(.rodata .rodata.*);
    }

    /* read/write data */
    .data : ALIGN(4) {
        *(.data .data.*);
    }

    /* zero-initialized read/write data */
    .bss : ALIGN(4) {
        __bss = .;
        *(.bss .bss.* .sbss .sbss.*);
        __bss_end = .;
    }

    . = ALIGN(4);
    . += 128 * 1024; /* 128KB */
    __stack_top = .;

    . = ALIGN(4 * 1024); /* 4KB pages */
    __free_ram = .;
    . += 64 * 1024 * 1024; /* 64MB */
    __free_ram_end = .;
}
