#!/bin/bash
# Build u-boot.img for Xiaomi Raphael (Redmi K20 Pro).
#
# Variants:
#   nolog  — keep ABL splash, logs on serial only (default)
#   log    — show U-Boot logs on the display
#
# Packages the Android boot image the way this device's ABL accepts:
#   gzip(u-boot-nodtb.bin) + u-boot.dtb, base 0x80000000
set -euo pipefail

VARIANT="${1:-nolog}"

cd "$(dirname "$0")"
export CROSS_COMPILE="${CROSS_COMPILE:-aarch64-linux-gnu-}"

case "$VARIANT" in
nolog)
	CONFIG_FRAGMENTS="qcom-phone.config raphael-phone.config"
	OUTPUT="boot_nolog.img"
	;;
log)
	CONFIG_FRAGMENTS="qcom-phone.config raphael-phone-log.config"
	OUTPUT="boot_log.img"
	;;
*)
	echo "Usage: $0 [nolog|log]" >&2
	exit 1
	;;
esac

make qcom_defconfig ${CONFIG_FRAGMENTS}
./scripts/config --set-str CONFIG_DEFAULT_DEVICE_TREE "qcom/sm8150-xiaomi-raphael"
./scripts/config --disable TOOLS_MKEFICAPSULE
./scripts/config --disable EFI_RUNTIME_UPDATE_CAPSULE
./scripts/config --disable EFI_CAPSULE_ON_DISK
./scripts/config --disable EFI_CAPSULE_FIRMWARE_RAW
make olddefconfig

make -j"$(nproc)"

gzip -kf u-boot-nodtb.bin
cat u-boot-nodtb.bin.gz u-boot.dtb > u-boot-nodtb.bin.gz-dtb

mkbootimg --kernel u-boot-nodtb.bin.gz-dtb \
	--pagesize 4096 \
	--base 0x80000000 \
	--output "$OUTPUT"

echo "Built: $(pwd)/$OUTPUT"
if command -v unpack_bootimg >/dev/null 2>&1; then
	unpack_bootimg --boot_img "$OUTPUT" 2>&1 | head -8
fi
