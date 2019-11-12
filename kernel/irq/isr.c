#include <irq/isr.h>
#include <irq/idt.h>
#include <utils/port.h>
#include <utils/print.h>
#include <mm/mm.h>

irq_handler_t irq_handlers[IDT_ENTRIES];

void irq_eoi(uint8_t irq) {
	if (irq >= 0x20 && irq < 0x30) {
		if (irq >= 0x28)
			outb(0xA0, 0x20);

		outb(0x20, 0x20);
	}
}

static const char *exc_names[] = {
		"#DE", "#DB", "NMI", "#BP", "#OF", "#BR", "#UD", "#NM",
		"#DF", "-",   "#TS", "#NP", "#SS", "#GP", "#PF", "-",
		"#MF", "#AC", "#MC", "#XM", "#VE", "-",   "-",   "-",
		"-",   "-",   "-",   "-",   "-",   "-",   "#SX", "-", "-"
};

void dispatch_interrupt(registers_t *state) {
    uint32_t irq = state->int_no;

	int success = 0;
	if (irq_handlers[irq])
		success = irq_handlers[irq](state);

    if (!success) {
        if (irq < 0x20) {
            if (state->cs == 0x08) {
                if(irq == 14) {
                    uintptr_t cr2 = 0;
					asm volatile ("mov %%cr2, %%rax" : "=a"(cr2) : : "memory");
					panic("Page fault: cr2: %08lx, rip: %08lx", cr2, state->rip);
                } else {
                    panic("Unhandled exception %s (%u) error %02x", exc_names[irq], irq, state->err);
                }
            } else {
                kprint("IRQ", "user mode panic!");
				kprint("IRQ", "how did we get here at this point!");
                if(irq == 14) {
                    uintptr_t cr2 = 0;
					asm volatile ("mov %%cr2, %%rax" : "=a"(cr2) : : "memory");
					panic("Page fault: cr2: %08lx, rip: %08lx", cr2, state->rip);
                } else {
                    panic("Unhandled exception %s (%u) error %02x", exc_names[irq], irq, state->err);
                }
            }
        }
    }

    irq_eoi(irq);
}

int isr_register_handler(uint8_t irq, irq_handler_t handler) {
	if (irq_handlers[irq]) return 0;
	irq_handlers[irq] = handler;
	return 1;
}

int isr_unregister_handler(uint8_t irq) {
	if (!irq_handlers[irq]) return 0;
	irq_handlers[irq] = NULL;
	return 1;
}