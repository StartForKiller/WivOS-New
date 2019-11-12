[BITS 32]

SECTION .multiboot
align 64

MAGIC equ 0x1BADB002
FLAGS equ 1 << 0 | 1 << 1 | 1 << 2
CHECKSUM equ -(MAGIC + FLAGS)

KERN_VBASE equ 0xFFFFFFFF80000000

multiboot_header:
	dd MAGIC
	dd FLAGS
	dd CHECKSUM

	dd 0
	dd 0
	dd 0
	dd 0
	dd 0

	dd 0
	dd 1024
	dd 768
	dd 32

SECTION .bss
align 0x1000

stack_bottom:
	resb 0x10000
stack_top:

SECTION .data

align 0x1000
init_pml4:
    dq init_pdp1 - KERN_VBASE + 3
    times 255 dq 0
    dq phys_pdp - KERN_VBASE + 3
    times 254 dq 0
    dq init_pdp2 - KERN_VBASE + 3

align 0x1000
init_pdp1:
	dq init_pd - KERN_VBASE + 3
	times 511 dq 0

align 0x1000
init_pdp2:
	times 510 dq 0
	dq init_pd - KERN_VBASE + 3
	dq 0

align 0x1000
phys_pdp:
	dq phys_pd - KERN_VBASE + 3
	times 511 dq 0

;Stolen from quack(https://gitlab.com/quack-os/quack/blob/master/kernel/arch/x86_64/boot/boot.asm)
%macro gen_pd_2mb 3
	%assign i %1
	%rep %2
		dq (i | 0x83)
		%assign i i+0x200000
	%endrep
	%rep %3
		dq 0
	%endrep
%endmacro

align 0x1000
init_pd:
	gen_pd_2mb 0,64,448

align 0x1000
phys_pd:
	gen_pd_2mb 0,512,0

align 0x10
stub_gdt64:
	.null: equ $ - stub_gdt64
		dq 0
	.code: equ $ - stub_gdt64
		dq (1<<43) | (1<<44) | (1<<47) | (1<<53)
	.pointer:
		dw $ - stub_gdt64 - 1
		dq stub_gdt64 - KERN_VBASE

SECTION .text
align 4

global _start
extern _long64_start
_start:
    lgdt [stub_gdt64.pointer - KERN_VBASE]

    mov esp, stack_top - KERN_VBASE

    push 0
	push eax
	push 0
	push ebx

    mov eax, 80000000h
    cpuid
    cmp eax, 80000000h
    jbe _no_long_mode
    mov eax, 80000001h
    cpuid
    bt edx, 29
    jnc _no_long_mode

    mov eax, cr4
    or eax, 0x000000A0
    mov cr4, eax

    mov eax, init_pml4 - KERN_VBASE
    mov cr3, eax

    mov ecx, 0xC0000080
    rdmsr
    or eax, 0x00000101
    wrmsr

    mov eax, cr0
    or eax, 0x80000001
    mov cr0, eax

    jmp stub_gdt64.code:higher_half_entry_lower

higher_half_entry_lower equ (higher_half_entry - KERN_VBASE)

_no_long_mode:
    cli
    hlt
    jmp _no_long_mode

bits 64
higher_half_entry:
	mov rax, _long64_start
	jmp rax