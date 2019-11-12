[BITS 64]

SECTION .data
align 0x10
stub_gdt64:
	.null: equ $ - stub_gdt64
		dq 0
	.code: equ $ - stub_gdt64
		dq (1<<43) | (1<<44) | (1<<47) | (1<<53)
	.pointer_high:
		dw $ - stub_gdt64 - 1
		dq stub_gdt64

SECTION .text

KERN_VBASE equ 0xFFFFFFFF80000000

global _long64_start
extern kernel_main
extern syscall_interrupt
_long64_start:
    cli
    lgdt [stub_gdt64.pointer_high]

	mov ax, 0
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	mov ss, ax
	
	mov rcx, 0xc0000101
    mov eax, edi
    shr rdi, 32
    mov edx, edi
    wrmsr

	mov rax, cr0
    and al, 0xfb
    or al, 0x02
    mov cr0, rax
    mov rax, cr4
    or ax, 3 << 9
    mov cr4, rax

    add rsp, KERN_VBASE
    pop rdi
	pop rsi
	call kernel_main

	cli
	hlt

bits 64
global mem_fast_memcpy
mem_fast_memcpy:
	push rdi
	push rsi
	push rdx
	mov rcx, rdx
	rep movsb
	pop rdx
	pop rsi
	pop rdi
	ret

global mem_fast_memset
mem_fast_memset:
	push rax
	push rdi
	push rsi
	push rdx
	mov rcx, rdx
	mov rax, rsi
	rep stosb
	pop rdx
	pop rsi
	pop rdi
	pop rax
	ret