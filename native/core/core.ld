ENTRY(numc3ds_entry)

SECTIONS
{
    . = 0;
    .native :
    {
        KEEP(*(.entry))
        *(.text*)
        *(.rodata*)
        *(.data*)
        *(.got*)
        *(.state*)
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
