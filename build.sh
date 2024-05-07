#!/usr/bin/env bash

set -e

cd "$(dirname $0)"

RKUEFIBUILDTYPE=${1}
shift
RKUEFIBOARDS=${1}
shift

export WORKSPACE="$PWD"
export PACKAGES_PATH=$PWD/edk2:$PWD/edk2-platforms:$PWD/edk2-non-osi:$PWD/edk2-rockchip
export GCC5_AARCH64_PREFIX=aarch64-linux-gnu-

TRUST_INI=RK3568TRUST.ini
MINIALL_INI=RK3568MINIALL.ini

RKBIN=edk2-rockchip-non-osi/rkbin
FIRMWARE_VER="$(git describe --tags --dirty)"

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
	echo " => Building UEFI"
	build -n $(getconf _NPROCESSORS_ONLN) -b ${RKUEFIBUILDTYPE} -a AARCH64 -t GCC5 \
	    -D FIRMWARE_VER="${FIRMWARE_VER}" \
	    -p Platform/${vendor}/${board}/${board}.dsc
}

build_idblock() {
	echo " => Building idblock.bin"
	FLASHFILES="FlashHead.bin FlashData.bin FlashBoot.bin"
	rm -f idblock.bin rk35*_ddr_*.bin rk356x_usbplug*.bin UsbHead.bin ${FLASHFILES}

	# Default DDR image uses 1.5M baud. Patch it to use 115200 to match UEFI first.
	cat `pwd`/${RKBIN}/tools/ddrbin_param.txt					 		\
		| sed 's/^uart baudrate=.*$/uart baudrate=115200/'  		\
		| sed 's/^dis_printf_training=.*$/dis_printf_training=1/' 	\
		> `pwd`/Build/ddrbin_param.txt
	./${RKBIN}/tools/ddrbin_tool `pwd`/Build/ddrbin_param.txt ${RKBIN}/${DDR}
	./${RKBIN}/tools/ddrbin_tool -g `pwd`/Build/ddrbin_param_dump.txt ${RKBIN}/${DDR}

	# Create idblock.bin
	(cd ${RKBIN} && ./tools/boot_merger RKBOOT/${MINIALL_INI})
	./${RKBIN}/tools/boot_merger unpack --loader ${RKBIN}/rk356x_spl_loader_*.bin --output .
	cat ${FLASHFILES} > idblock.bin
	git checkout ${RKBIN}/${DDR}

	# Cleanup
	rm -f ${RKBIN}/rk356x_spl_loader_*.bin
	rm -f rk35*_ddr_*.bin rk356x_usbplug*.bin UsbHead.bin ${FLASHFILES}
}

build_fit() {
	board=$1
	type=$2
	board_upper=`echo $board | tr '[:lower:]' '[:upper:]'`
	echo " => Building FIT"
	./scripts/extractbl31.py ${RKBIN}/${BL31}
	cat uefi.its | sed "s,@BOARDTYPE@,${type},g" > ${board_upper}_EFI.its
	./${RKBIN}/tools/mkimage -f ${board_upper}_EFI.its -E ${board_upper}_EFI.itb
	dd if=Build/${board}/${RKUEFIBUILDTYPE}_GCC5/FV/RK356X_EFI.fd of=${board_upper}_EFI.itb bs=512 seek=$((1024 * 1024 / 512))
	rm -f bl31_0x*.bin ${board_upper}_EFI.its
}

fetch_deps

BL31=$(grep '^PATH=.*_bl31_' ${RKBIN}/RKTRUST/${TRUST_INI} | cut -d = -f 2-)
DDR=$(grep '^Path1=.*_ddr_' ${RKBIN}/RKBOOT/${MINIALL_INI} | cut -d = -f 2-)

test -r ${RKBIN}/${BL31} || (echo "${RKBIN}/${BL31} not found"; false)
. edk2/edksetup.sh

build_uefitools

for board in ${RKUEFIBOARDS}; do
	case ${board} in
	QUARTZ64)
		build_uefi Pine64 Quartz64
		build_fit Quartz64 rk3566-quartz64-a
		;;
	SOQUARTZ)
		build_uefi Pine64 SOQuartz
		build_fit SOQuartz rk3566-soquartz-cm4
		;;
	ROC-RK3566-PC)
		build_uefi Firefly ROC-RK3566-PC
		build_fit ROC-RK3566-PC rk3566-roc-pc
		;;
	ROC-RK3568-PC)
		build_uefi Firefly ROC-RK3568-PC
		build_fit ROC-RK3568-PC rk3568-firefly-roc-pc
		;;
	ORANGEPI3B)
		build_uefi OrangePi OrangePi3B
		build_fit OrangePi3B rk3566-orangepi-3b
		;;
	PINETAB2)
		build_uefi Pine64 PineTab2
		build_fit PineTab2 rk3566-pinetab2
		;;
	ZERO-3W)
		build_uefi Radxa Zero-3W
		build_fit Zero-3W rk3566-radxa-zero-3w
		;;
	*)
		echo "Unknown board ${board}"
		exit 1
		;;
	esac
done

build_idblock

echo
ls -l `pwd`/idblock.bin `pwd`/*_EFI.itb
echo
