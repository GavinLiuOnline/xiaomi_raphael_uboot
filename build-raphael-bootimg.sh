#!/bin/bash
# Build u-boot.img for Xiaomi Raphael (Redmi K20 Pro).
#
# Uses qcom-phone drivers/env (buttons, boot menu, bootefi bootmgr) but packages
# the Android boot image the way this device's ABL accepts:
#   gzip(u-boot-nodtb.bin) + u-boot.dtb, base 0x80000000
set -euo pipefail

cd "$(dirname "$0")"
export CROSS_COMPILE="${CROSS_COMPILE:-aarch64-linux-gnu-}"

make qcom_defconfig qcom-phone.config raphael-phone.config
./scripts/config --set-str CONFIG_DEFAULT_DEVICE_TREE "qcom/sm8150-xiaomi-raphael"
./scripts/config --disable TOOLS_MKEFICAPSULE
./scripts/config --disable EFI_RUNTIME_UPDATE_CAPSULE
./scripts/config --disable EFI_CAPSULE_ON_DISK
./scripts/config --disable EFI_CAPSULE_FIRMWARE_RAW

make -j"$(nproc)"

gzip -kf u-boot-nodtb.bin
cat u-boot-nodtb.bin.gz u-boot.dtb > u-boot-nodtb.bin.gz-dtb

mkbootimg --kernel u-boot-nodtb.bin.gz-dtb \
	--pagesize 4096 \
	--base 0x80000000 \
	--output u-boot.img

cp -f u-boot.img boot.img
echo "Built: $(pwd)/u-boot.img"
unpack_bootimg --boot_img u-boot.img 2>&1 | head -8
