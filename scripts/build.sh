#!/usr/bin/env bash
#
# scripts/build.sh
# Build helper script for Edge ML Linux OS (Track A)
#

set -euo pipefail

# Buildroot strictly rejects any spaces in PATH (common in WSL when Windows PATH is imported).
# Filter out PATH entries containing spaces:
CLEAN_PATH=""
IFS=':' read -ra PATH_ENTRIES <<< "$PATH"
for entry in "${PATH_ENTRIES[@]}"; do
    if [[ "$entry" != *" "* ]]; then
        if [ -z "$CLEAN_PATH" ]; then
            CLEAN_PATH="$entry"
        else
            CLEAN_PATH="${CLEAN_PATH}:${entry}"
        fi
    fi
done
export PATH="$CLEAN_PATH"

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
export BR2_DL_DIR="${REPO_ROOT}/downloads"
BR_DIR="${REPO_ROOT}/buildroot/buildroot-2025.02.10"
OUTPUT_RPI4="${REPO_ROOT}/output"
OUTPUT_QEMU="${REPO_ROOT}/output_qemu"

RPI4_DEFCONFIG="${REPO_ROOT}/configs/raspberrypi4_64_edgeml_defconfig"
QEMU_DEFCONFIG="${REPO_ROOT}/configs/qemu_aarch64_virt_edgeml_defconfig"

print_usage() {
    echo "================================================================="
    echo "  Edge ML Linux OS (Track A) Build Helper"
    echo "================================================================="
    echo "Usage: $0 <command> [target]"
    echo ""
    echo "Commands:"
    echo "  configure [rpi4|qemu]   Load defconfig into the out-of-tree output dir"
    echo "  check-config [rpi4|qemu] Print and verify core configuration variables"
    echo "  build [rpi4|qemu] [jobs] Build target (default: 4 parallel jobs)"
    echo "  run-qemu                Run QEMU emulation (for qemu target)"
    echo "  menuconfig [rpi4|qemu]  Open interactive Buildroot Kconfig menu"
    echo "  savedefconfig [rpi4]    Save current .config back to defconfig"
    echo "  clean [rpi4|qemu]       Remove output build directory"
    echo ""
    echo "Targets:"
    echo "  rpi4  - Raspberry Pi 4 Model B (4GB RAM) [Default]"
    echo "  qemu  - Generic ARM64 QEMU Virt Emulation Target"
    echo "================================================================="
}

TARGET="${2:-rpi4}"

if [ "$TARGET" = "rpi4" ]; then
    OUT_DIR="$OUTPUT_RPI4"
    DEFCONFIG_FILE="$RPI4_DEFCONFIG"
    TARGET_NAME="Raspberry Pi 4 Model B (4GB, ARM64)"
elif [ "$TARGET" = "qemu" ]; then
    OUT_DIR="$OUTPUT_QEMU"
    DEFCONFIG_FILE="$QEMU_DEFCONFIG"
    TARGET_NAME="QEMU ARM64 Virt Emulation Target"
else
    echo "[ERROR] Unknown target: $TARGET (use 'rpi4' or 'qemu')"
    exit 1
fi

check_buildroot() {
    if [ ! -d "$BR_DIR" ]; then
        echo "[ERROR] Buildroot directory not found: $BR_DIR"
        echo "Please extract the archive: tar -xzf buildroot/buildroot-2025.02.10.tar.gz -C buildroot/"
        exit 1
    fi
}

CMD="${1:-help}"

case "$CMD" in
    configure)
        check_buildroot
        echo ">>> Configuring for: $TARGET_NAME"
        echo ">>> Using defconfig: $DEFCONFIG_FILE"
        echo ">>> Output dir:     $OUT_DIR"
        mkdir -p "$OUT_DIR"
        make -C "$BR_DIR" O="$OUT_DIR" BR2_DEFCONFIG="$DEFCONFIG_FILE" defconfig
        echo ">>> Configuration complete! (.config written to $OUT_DIR/.config)"
        ;;

    check-config)
        if [ ! -f "$OUT_DIR/.config" ]; then
            echo "[ERROR] $OUT_DIR/.config not found. Run '$0 configure $TARGET' first."
            exit 1
        fi
        echo "================================================================="
        echo "  Verifying Configuration: $TARGET_NAME"
        echo "================================================================="
        grep -E 'BR2_ARCH=|BR2_cortex_a72=|BR2_TOOLCHAIN_BUILDROOT_GLIBC=|BR2_INIT_BUSYBOX=|BR2_TOOLCHAIN_BUILDROOT_CXX=|BR2_GCC_VERSION=' "$OUT_DIR/.config" || true
        echo "================================================================="
        ;;

    build)
        check_buildroot
        if [ ! -f "$OUT_DIR/.config" ]; then
            $0 configure "$TARGET"
        fi
        JOBS="${3:-4}"
        echo ">>> Building $TARGET_NAME using $JOBS parallel jobs..."
        make -C "$BR_DIR" O="$OUT_DIR" -j"$JOBS"
        echo ">>> Build completed successfully for $TARGET_NAME!"
        ;;

    run-qemu)
        if [ ! -f "$OUTPUT_QEMU/images/Image" ] || [ ! -f "$OUTPUT_QEMU/images/rootfs.ext4" ]; then
            echo "[ERROR] Kernel or rootfs image not found. Run '$0 build qemu' first."
            exit 1
        fi
        echo ">>> Launching QEMU AArch64 Virt Emulation..."
        echo ">>> (To exit QEMU at any time: Press 'Ctrl + A', release, then press 'X')"
        echo ">>> Login with: root (no password required)"
        echo "================================================================="
        qemu-system-aarch64 \
            -M virt \
            -cpu cortex-a53 \
            -nographic \
            -smp 1 \
            -kernel "$OUTPUT_QEMU/images/Image" \
            -append "rootwait root=/dev/vda console=ttyAMA0" \
            -netdev user,id=eth0 \
            -device virtio-net-device,netdev=eth0 \
            -drive file="$OUTPUT_QEMU/images/rootfs.ext4",if=none,format=raw,id=hd0 \
            -device virtio-blk-device,drive=hd0
        ;;

    menuconfig)
        check_buildroot
        if [ ! -f "$OUT_DIR/.config" ]; then
            $0 configure "$TARGET"
        fi
        make -C "$BR_DIR" O="$OUT_DIR" menuconfig
        ;;

    savedefconfig)
        check_buildroot
        if [ ! -f "$OUT_DIR/.config" ]; then
            echo "[ERROR] $OUT_DIR/.config not found."
            exit 1
        fi
        echo ">>> Saving defconfig back to $DEFCONFIG_FILE..."
        make -C "$BR_DIR" O="$OUT_DIR" BR2_DEFCONFIG="$DEFCONFIG_FILE" savedefconfig
        ;;

    clean)
        echo ">>> Removing $OUT_DIR..."
        rm -rf "$OUT_DIR"
        echo ">>> Cleaned."
        ;;

    help|--help|-h)
        print_usage
        ;;

    *)
        echo "[ERROR] Unknown command: $CMD"
        print_usage
        exit 1
        ;;
esac
