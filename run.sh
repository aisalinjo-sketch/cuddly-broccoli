#!/bin/bash

# Novaria OS Kernel - Run Script for QEMU

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build"
ISO_FILE="${BUILD_DIR}/novaria-0.1.0.iso"
KERNEL_ELF="${BUILD_DIR}/novaria-kernel.elf"

# Check for OVMF (UEFI firmware)
OVMF_PATHS=(
    "/usr/share/qemu/OVMF.fd"
    "/usr/share/ovmf/OVMF.fd"
    "/usr/local/share/qemu/OVMF.fd"
    "/opt/homebrew/share/qemu/OVMF.fd"
)

OVMF=""
for path in "${OVMF_PATHS[@]}"; do
    if [[ -f "${path}" ]]; then
        OVMF="${path}"
        break
    fi
done

if [[ -z "${OVMF}" ]]; then
    echo "ERROR: OVMF.fd not found. Install qemu-system-x86_64 or edk2-ovmf"
    exit 1
fi

# Check what to run
if [[ -f "${ISO_FILE}" ]]; then
    echo "Running from ISO: ${ISO_FILE}"
    qemu-system-x86_64 \
        -cdrom "${ISO_FILE}" \
        -serial stdio \
        -display default \
        -bios "${OVMF}" \
        -m 256M \
        -smp 2
elif [[ -f "${KERNEL_ELF}" ]]; then
    echo "Running kernel directly (multiboot): ${KERNEL_ELF}"
    qemu-system-x86_64 \
        -kernel "${KERNEL_ELF}" \
        -serial stdio \
        -display default \
        -bios "${OVMF}" \
        -m 256M \
        -smp 2
else
    echo "ERROR: No kernel or ISO found. Run ./build.sh first."
    exit 1
fi