#!/bin/bash
set -euo pipefail

INPUT="$1"
MOUNT_PATH="$2"
DEVICE=""
mounted=false

cleanup() {
    if [ "$mounted" = true ]; then
        sudo umount "$MOUNT_PATH" || true
    fi

    if [[ -n "$DEVICE" ]]; then
        sudo losetup -d "$DEVICE" || true
    fi

    sudo rm -rf "$MOUNT_PATH"
}
trap cleanup EXIT

sudo mkdir -p "$MOUNT_PATH"

DEVICE=$(sudo losetup -fP --show "$INPUT")
PARTITION="${DEVICE}p1"

sudo mount "$PARTITION" "$MOUNT_PATH"
mounted=true

read -p "Press enter to unmount..."
