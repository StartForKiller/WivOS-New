ENTRY(_start)
OUTPUT_FORMAT(elf64-x86-64)

KERN_LMA = 0x0000000000200000;
KERN_VMA = 0xFFFFFFFF80000000;

SECTIONS
{
    
		. = KERN_LMA + KERN_VMA;

		kernel_start = . ;

		.text : AT(ADDR(.text) - KERN_VMA)
		{
			*(.multiboot*)
			*(.text*)
			*(.rodata*)
		}

		.data ALIGN (0x1000) : AT(ADDR(.data) - KERN_VMA)
		{
			*(.data*)
		}

		.bss : AT(ADDR(.bss) - KERN_VMA)
		{
			*(COMMON)
			*(.bss*)
		}

		kernel_end = . ;

}