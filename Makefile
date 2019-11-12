all: run

iso:
	make -C kernel
	@mkdir -p isodir/boot/grub
	@cp kernel/wiv.elf isodir/boot/wiv.elf
	@cp kernel/boot/grub.cfg isodir/boot/grub/grub.cfg
	@grub-mkrescue -o wivos.iso isodir > /dev/null

run: iso
	qemu-system-x86_64 -drive file=wivos.iso,index=0,media=disk,format=raw -no-shutdown -no-reboot -enable-kvm -m 4096M -serial stdio

clean:
	make -C kernel clean
	@rm -fr isodir
	@rm -fr wivos.iso