#!/bin/bash

# Novaria OS Kernel - Build Script

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build"
KERNEL_NAME="novaria-kernel"

echo "=== Novaria OS Kernel Build ==="
echo "Source: ${SCRIPT_DIR}"
echo "Build:  ${BUILD_DIR}"

# Create build directory
mkdir -p "${BUILD_DIR}"
cd "${BUILD_DIR}"

# Configure with CMake using toolchain file
echo "Configuring with CMake..."
cmake "${SCRIPT_DIR}" -DCMAKE_TOOLCHAIN_FILE="${SCRIPT_DIR}/toolchain.cmake" -DCMAKE_BUILD_TYPE=Release

# Build (use sysctl for CPU count on macOS)
CPU_COUNT=$(sysctl -n hw.ncpu 2>/dev/null || echo 4)
echo "Building kernel with ${CPU_COUNT} jobs..."
make -j${CPU_COUNT}

# Check if kernel was built
KERNEL_ELF="${BUILD_DIR}/${KERNEL_NAME}.elf"
if [[ -f "${KERNEL_ELF}" ]]; then
    echo "Kernel built successfully: ${KERNEL_ELF}"
    ls -lh "${KERNEL_ELF}"
else
    echo "ERROR: Kernel not found at ${KERNEL_ELF}"
    exit 1
fi

# Create ISO if grub-mkrescue is available
if command -v grub-mkrescue &> /dev/null; then
    echo "Creating bootable ISO..."
    mkdir -p iso/boot/grub
    cp "${KERNEL_ELF}" iso/boot/kernel.elf
    cp "${SCRIPT_DIR}/grub.cfg" iso/boot/grub/grub.cfg
    grub-mkrescue -o "${BUILD_DIR}/novaria-0.1.0.iso" iso 2>/dev/null
    echo "ISO created: ${BUILD_DIR}/novaria-0.1.0.iso"
else
    echo "grub-mkrescue not found, skipping ISO creation"
    echo "Install grub2-tools to create bootable ISO"
fi

echo "=== Build Complete ==="