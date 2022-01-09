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
	parted -s sdcard.img unit s mkpart loader 64 8MiB
	parted -s sdcard.img unit s mkpart uboot 8MiB 16MiB
	parted -s sdcard.img unit s mkpart env 16MiB 32MiB

	# Quartz64 boards
	for size in 4GB 8GB; do							\
		cp sdcard.img sdcard_Quartz64_$${size}.img;			\
		dd if=idblock.bin of=sdcard_Quartz64_$${size}.img 		\
		    seek=64 conv=notrunc;					\
		dd if=QUARTZ64_EFI_$${size}.itb of=sdcard_Quartz64_$${size}.img	\
		    seek=20480 conv=notrunc;					\
	done
	# SOQuartz modules
	for size in 2GB 4GB 8GB; do						\
		cp sdcard.img sdcard_SOQuartz_$${size}.img;			\
		dd if=idblock.bin of=sdcard_SOQuartz_$${size}.img 		\
		    seek=64 conv=notrunc;					\
		dd if=SOQUARTZ_EFI_$${size}.itb of=sdcard_SOQuartz_$${size}.img	\
		    seek=20480 conv=notrunc;					\
	done
	# ROC-RK3566-PC boards
	for size in 4GB; do							\
		cp sdcard.img sdcard_ROC-RK3566-PC_$${size}.img;		\
		dd if=idblock.bin of=sdcard_ROC-RK3566-PC_$${size}.img 		\
		    seek=64 conv=notrunc;					\
		dd if=ROC-RK3566-PC_EFI_$${size}.itb of=sdcard_ROC-RK3566-PC_$${size}.img \
		    seek=20480 conv=notrunc;					\
	done
	rm -f sdcard.img

.PHONY: release
release: sdcard
	rm -f sdcard_*_?GB.img.gz
	gzip sdcard_*_?GB.img

.PHONY: clean
clean:
	rm -rf Build
	rm -f bl31_*.bin
	rm -f idblock.bin
	rm -f QUARTZ64_EFI_*.itb
	rm -f SOQUARTZ_EFI_*.itb
	rm -f ROC-RK3566-PC_EFI_*.itb
	rm -f .uefitools_done
	rm -f sdcard_*.img
