BOARDS ?= QUARTZ64 SOQUARTZ ROC-RK3566-PC ROC-RK3568-PC ORANGEPI3B PINETAB2 NANOPI-R5S
TARGET ?= RELEASE

.PHONY: all
all: uefi

.PHONY: uefi
uefi:
	@./build.sh $(TARGET) "$(BOARDS)"

.PHONY: sdcard
sdcard: uefi
	rm -f sdcard.img
	fallocate -l 33M sdcard.img
	parted -s sdcard.img mklabel gpt
	parted -s sdcard.img unit s mkpart loader 64 8MiB
	parted -s sdcard.img unit s mkpart uboot 8MiB 16MiB
	parted -s sdcard.img unit s mkpart env 16MiB 32MiB

	for board in $(BOARDS); do							\
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
