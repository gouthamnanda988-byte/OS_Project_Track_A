#!/usr/bin/env bash
#
# scripts/build.sh
# Build helper script for Edge ML Linux OS (Track A)
#

set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
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
