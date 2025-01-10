ENTRY(start)

SECTIONS {
    . = 0x1000000;

    /* program code */
    .text : {
        KEEP(*(.text.start));
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
        *(.bss .bss.* .sbss .sbss.*);

        . = ALIGN(16);
        . += 64 * 1024; /* 64KB stack */
        __stack_top = .;

        ASSERT(. < 0x1800000, "too large executable");
    }
}
