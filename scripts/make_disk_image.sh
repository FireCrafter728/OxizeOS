#!/bin/bash

TARGET=$1
SIZE=$2
STAGE2_LOCATION_OFFSET=480
DISK_SECTOR_COUNT=$(( (${SIZE} + 511) / 512 ))
STAGE2_SIZE=$(stat -c%s ${output}/stage2.bin)
STAGE2_SECTORS=$(( (${STAGE2_SIZE} + 511) / 512 ))
DISK_PART1_BEGIN=2048
DISK_PART1_END=$(( ${DISK_SECTOR_COUNT} - 1 ))
MOUNT_PATH=/tmp/OxizeOS

dd if=/dev/zero of=$TARGET bs=512 count=${DISK_SECTOR_COUNT} >/dev/null 2>&1

parted -s $TARGET mklabel msdos
parted -s $TARGET mkpart primary ${DISK_PART1_BEGIN}s ${DISK_PART1_END}s
parted -s $TARGET set 1 boot on

if [ ${STAGE2_SECTORS} -gt 2047 ]; then
    echo -e "\033[31mStage2.bin is larger than 2047 sectors!"
    exit 2
fi

dd if=${output}/stage2.bin of=$TARGET conv=notrunc bs=512 seek=1 >/dev/null 2>&1

DEVICE=$(sudo losetup -fP --show $TARGET)
TARGET_PART="${DEVICE}p1"

sudo mkfs.fat -F 32 -n "OXIZEOS" $TARGET_PART >/dev/null

sudo dd if=${output}/stage1.bin of=$TARGET_PART conv=notrunc bs=1 count=3 >/dev/null 2>&1
sudo dd if=${output}/stage1.bin of=$TARGET_PART conv=notrunc bs=1 seek=90 skip=90 >/dev/null 2>&1

echo "01 00 00 00" | xxd -r -p | sudo dd of=$TARGET_PART conv=notrunc bs=1 seek=$STAGE2_LOCATION_OFFSET >/dev/null 2>&1
printf "%x" ${STAGE2_SECTORS} | xxd -r -p | sudo dd of=$TARGET_PART conv=notrunc bs=1 seek=$(( STAGE2_LOCATION_OFFSET + 4 )) >/dev/null 2>&1

sudo mkdir -p $MOUNT_PATH

sudo mount ${TARGET_PART} $MOUNT_PATH

sudo mkdir -p ${MOUNT_PATH}/BOOT
sudo mkdir -p ${MOUNT_PATH}/BOOT/BIOS
sudo cp ${output}/kernel.bin ${MOUNT_PATH}/BOOT/BIOS
sudo mkdir -p ${MOUNT_PATH}/BOOT/BIOS/tests
sudo cp test.txt ${MOUNT_PATH}/BOOT/BIOS/tests
sudo cp LFNTestFile.txt ${MOUNT_PATH}/BOOT/BIOS/tests
sudo mkdir -p ${MOUNT_PATH}/BOOT/KERNEL
sudo cp ${output}/x86Kern.exe ${MOUNT_PATH}/BOOT/KERNEL

sudo umount ${MOUNT_PATH}

sudo losetup -d ${DEVICE}