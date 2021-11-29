.PHONY: all
all: uefi

.PHONY: uefi
uefi:
	@./build.sh

.PHONY: sdcard
sdcard: uefi
	rm -f sdcard.img
	fallocate -l 33M sdcard.img
	parted -s sdcard.img mklabel gpt
	parted -s sdcard.img unit s mkpart loader 64 8MiB
	parted -s sdcard.img unit s mkpart uboot 8MiB 16MiB
	parted -s sdcard.img unit s mkpart env 16MiB 32MiB

	for size in 4GB 8GB; do						\
		cp sdcard.img sdcard_$${size}.img;			\
		dd if=idblock.bin of=sdcard_$${size}.img 		\
		    seek=64 conv=notrunc;				\
		dd if=QUARTZ64_EFI_$${size}.itb of=sdcard_$${size}.img	\
		    seek=20480 conv=notrunc;				\
	done
	rm -f sdcard.img

.PHONY: release
release: sdcard
	gzip sdcard_4GB.img
	gzip sdcard_8GB.img

.PHONY: clean
clean:
	rm -rf Build
	rm -f bl31_*.bin
	rm -f idblock.bin
	rm -f QUARTZ64_EFI_*.itb
	rm -f .uefitools_done
	rm -f sdcard_*.img
