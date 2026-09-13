ENTRY(numc3ds_build_splashes)

SECTIONS
{
    . = 0x0045DAF0;
    .splash_loader :
    {
        KEEP(*(.entry))
        *(.text*)
        *(.rodata*)
        *(.data*)
        *(.bss*)
        *(COMMON)
    }

    /DISCARD/ :
    {
        *(.ARM.attributes)
        *(.comment)
        *(.note*)
        *(.eh_frame*)
    }
}
