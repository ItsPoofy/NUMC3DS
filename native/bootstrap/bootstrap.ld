ENTRY(numc3ds_bootstrap)

SECTIONS
{
    . = 0x00918940;
    .bootstrap :
    {
        KEEP(*(.entry))
        KEEP(*(.patch_targets))
        *(.text*)
        *(.rodata*)
        *(.data*)
        *(.bss*)
        *(COMMON)
    }

    .call_veneers 0x00918FE0 :
    {
        KEEP(*(.call_veneers))
    }

    /DISCARD/ :
    {
        *(.ARM.attributes)
        *(.comment)
        *(.note*)
        *(.eh_frame*)
    }
}
