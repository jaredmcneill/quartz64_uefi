.PHONY: all
all: uefi_debug

.PHONY: uefi_release
uefi_release:
	@./build.sh RELEASE

.PHONY: uefi_debug
uefi_debug:
	@./build.sh DEBUG

.PHONY: sdcard
sdcard: uefi_release
	rm -f sdcard.img
	fallocate -l 33M sdcard.img
	parted -s sdcard.img mklabel gpt
	parted -s sdcard.img mkpart loader 64s 8MiB
	parted -s sdcard.img mkpart uboot 8MiB 16MiB
	parted -s sdcard.img mkpart env 16MiB 32MiB

	for board in QUARTZ64 SOQUARTZ ROC-RK3566-PC; do				\
		cp sdcard.img $${board}_EFI.img;				\
		dd if=idblock.bin of=$${board}_EFI.img 			\
		    seek=64 conv=notrunc;						\
		dd if=$${board}_EFI.itb of=$${board}_EFI.img	\
		    seek=20480 conv=notrunc;					\
	done
	rm -f sdcard.img

.PHONY: release
release: sdcard
	rm -f *_EFI.img.gz
	gzip *_EFI.img

.PHONY: clean
clean:
	rm -rf Build
	rm -f bl31_*.bin
	rm -f idblock.bin
	rm -f *.itb
	rm -f .uefitools_done
	rm -f *_EFI.img *_EFI.img.gz
