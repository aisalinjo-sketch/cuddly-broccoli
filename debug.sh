#!/bin/bash

# Novaria OS Kernel - Debug Script (with GDB)

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build"
ISO_FILE="${BUILD_DIR}/novaria-0.1.0.iso"
KERNEL_ELF="${BUILD_DIR}/novaria-kernel.elf"

# Check for OVMF
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
    echo "ERROR: OVMF.fd not found"
    exit 1
fi

# Determine what to debug
if [[ -f "${ISO_FILE}" ]]; then
    KERNEL_FILE="${ISO_FILE}"
    QEMU_ARGS="-cdrom ${ISO_FILE}"
elif [[ -f "${KERNEL_ELF}" ]]; then
    KERNEL_FILE="${KERNEL_ELF}"
    QEMU_ARGS="-kernel ${KERNEL_ELF}"
else
    echo "ERROR: No kernel or ISO found. Run ./build.sh first."
    exit 1
fi

echo "Starting QEMU with GDB server on port 1234..."
echo "Kernel: ${KERNEL_FILE}"
echo ""
echo "In another terminal, run:"
echo "  gdb ${KERNEL_ELF}"
echo "  (gdb) target remote :1234"
echo "  (gdb) break kernel_main"
echo "  (gdb) continue"
echo ""

qemu-system-x86_64 \
    ${QEMU_ARGS} \
    -serial stdio \
    -display default \
    -bios "${OVMF}" \
    -m 256M \
    -smp 2 \
    -s -S