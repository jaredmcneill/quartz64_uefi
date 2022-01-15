#!/usr/bin/env bash

set -e

cd "$(dirname $0)"

RKUEFIBUILDTYPE=${1}
shift

export WORKSPACE="$PWD"
export PACKAGES_PATH=$PWD/edk2:$PWD/edk2-platforms:$PWD/edk2-non-osi:$PWD/edk2-rockchip
export GCC5_AARCH64_PREFIX=aarch64-linux-gnu-

fetch_deps() {
	git submodule update --init --recursive
}

build_uefitools() {
	[ -r .uefitools_done ] && return
	echo " => Building UEFI tools"
	make -C edk2/BaseTools -j$(getconf _NPROCESSORS_ONLN) && touch .uefitools_done
}

build_uefi() {
	vendor=$1
	board=$2
	memsize=$3
	echo " => Building UEFI (PcdSystemMemorySize=${memsize})"
	build -n $(getconf _NPROCESSORS_ONLN) -b ${RKUEFIBUILDTYPE} -a AARCH64 -t GCC5 \
	    --pcd gArmTokenSpaceGuid.PcdSystemMemorySize=${memsize} \
	    -p Platform/${vendor}/${board}/${board}.dsc
}

build_idblock() {
	echo " => Building idblock.bin"
	FLASHFILES="FlashHead.bin FlashData.bin FlashBoot.bin"
	rm -f idblock.bin rk3566_ddr_*.bin rk356x_usbplug*.bin UsbHead.bin ${FLASHFILES}

	# Default DDR image uses 1.5M baud. Patch it to use 115200 to match UEFI first.
	cat rkbin/tools/ddrbin_param.txt | sed 's/^uart baudrate=/&115200/' > ddrbin_param.txt
	./rkbin/tools/ddrbin_tool ddrbin_param.txt rkbin/${DDR}
	rm -f ddrbin_param.txt

	# Create idblock.bin
	(cd rkbin && ./tools/boot_merger RKBOOT/RK3566MINIALL.ini)
	./rkbin/tools/boot_merger unpack --loader rkbin/rk356x_spl_loader_*.bin --output .
	cat ${FLASHFILES} > idblock.bin
	(cd rkbin && git checkout ${DDR})

	# Cleanup
	rm -f rkbin/rk356x_spl_loader_*.bin
	rm -f rk3566_ddr_*.bin rk356x_usbplug*.bin UsbHead.bin ${FLASHFILES}
}

build_fit() {
	board=$1
	board_upper=`echo $board | tr '[:lower:]' '[:upper:]'`
	tag=$2
	echo " => Building FIT (${tag})"
	./scripts/extractbl31.py rkbin/${BL31}
	cp -f Build/${board}/${RKUEFIBUILDTYPE}_GCC5/FV/RK356X_EFI.fd Build/RK356X_EFI.fd
	./rkbin/tools/mkimage -f uefi_${board}.its -E ${board_upper}_EFI_${tag}.itb
	rm -f bl31_0x*.bin Build/RK356X_EFI.fd
}

fetch_deps

BL31=$(grep '^PATH=.*_bl31_' rkbin/RKTRUST/RK3568TRUST.ini | cut -d = -f 2-)
DDR=$(grep '^Path1=.*_ddr_' rkbin/RKBOOT/RK3566MINIALL.ini | cut -d = -f 2-)

test -r rkbin/${BL31} || (echo "rkbin/${BL31} not found"; false)
. edk2/edksetup.sh

build_uefitools

# ROC-RK3566-PC boards
build_uefi Firefly ROC-RK3566-PC 0xF0000000
build_fit ROC-RK3566-PC 4GB

build_idblock

echo
ls -l `pwd`/idblock.bin `pwd`/*_EFI_*.itb
echo
