#!/usr/bin/env bash
#
# scripts/check_host_env.sh
# Verifies host environment and prerequisites for building Buildroot
# for Raspberry Pi 4 Model B (ARM64) with TensorFlow Lite.
#

set -euo pipefail

echo "================================================================="
echo "  Edge ML Linux OS (Track A) - Host Environment Check"
echo "================================================================="

FAILED=0
WARNINGS=0

check_cmd() {
    local cmd="$1"
    local required="$2"
    if command -v "$cmd" >/dev/null 2>&1; then
        local version
        version=$("$cmd" --version 2>&1 | head -n 1 || true)
        echo "[OK] Found $cmd ($version)"
    else
        if [ "$required" = "true" ]; then
            echo "[ERROR] Required tool missing: $cmd"
            FAILED=$((FAILED + 1))
        else
            echo "[WARN] Recommended tool missing: $cmd"
            WARNINGS=$((WARNINGS + 1))
        fi
    fi
}

echo ""
echo "--- 1. Architecture & OS ---"
OS=$(uname -s)
ARCH=$(uname -m)
echo "Operating System: $OS"
echo "Architecture:     $ARCH"
if [ "$OS" != "Linux" ]; then
    echo "[ERROR] Buildroot requires a Linux host environment."
    FAILED=$((FAILED + 1))
else
    echo "[OK] Host OS is Linux."
fi

if [ -f /etc/os-release ]; then
    . /etc/os-release
    echo "Distribution:     $PRETTY_NAME"
fi

echo ""
echo "--- 2. Mandatory Build Tools ---"
check_cmd make true
check_cmd gcc true
check_cmd g++ true
check_cmd patch true
check_cmd gzip true
check_cmd bzip2 true
check_cmd tar true
check_cmd cpio true
check_cmd unzip true
check_cmd rsync true
check_cmd file true
check_cmd bc true
check_cmd find true
check_cmd wget true
check_cmd python3 true
check_cmd git true

echo ""
echo "--- 3. Configuration & Optional Tools ---"
check_cmd pkg-config false
check_cmd ncurses6-config false
check_cmd flex false
check_cmd bison false

echo ""
echo "--- 4. Hardware Resources ---"
CPUS=$(nproc 2>/dev/null || echo 1)
TOTAL_RAM_KB=$(grep MemTotal /proc/meminfo | awk '{print $2}')
TOTAL_RAM_GB=$((TOTAL_RAM_KB / 1024 / 1024))
FREE_DISK_KB=$(df -k . | awk 'NR==2 {print $4}')
FREE_DISK_GB=$((FREE_DISK_KB / 1024 / 1024))

echo "CPU Cores:        $CPUS"
echo "Total RAM:        ${TOTAL_RAM_GB} GB"
echo "Available Disk:   ${FREE_DISK_GB} GB"

if [ "$FREE_DISK_GB" -lt 15 ]; then
    echo "[WARN] Less than 15 GB disk space available. Buildroot + toolchain + kernel may require 15-25 GB."
    WARNINGS=$((WARNINGS + 1))
else
    echo "[OK] Disk space is sufficient (>= 15 GB)."
fi

if [ "$TOTAL_RAM_GB" -lt 4 ]; then
    echo "[WARN] Less than 4 GB RAM. Large C++ compilation (e.g. TensorFlow Lite) might need swap space."
    WARNINGS=$((WARNINGS + 1))
else
    echo "[OK] RAM is sufficient (>= 4 GB)."
fi

echo ""
echo "--- 5. Pinned Source Tarballs ---"
EXPECTED_BR_TAR="buildroot/buildroot-2025.02.10.tar.gz"
EXPECTED_BR_SHA256="f3de928ed368ef3dcadb97d4f4c1a9f49424a367759dba1d97c4c2bc5a22eab1"

if [ -f "$EXPECTED_BR_TAR" ]; then
    echo "[OK] Found $EXPECTED_BR_TAR"
    ACTUAL_SHA256=$(sha256sum "$EXPECTED_BR_TAR" | awk '{print $1}')
    if [ "$ACTUAL_SHA256" = "$EXPECTED_BR_SHA256" ]; then
        echo "[OK] SHA256 checksum matches: $ACTUAL_SHA256"
    else
        echo "[ERROR] Checksum mismatch for $EXPECTED_BR_TAR!"
        echo "  Expected: $EXPECTED_BR_SHA256"
        echo "  Actual:   $ACTUAL_SHA256"
        FAILED=$((FAILED + 1))
    fi
else
    echo "[ERROR] Missing pinned Buildroot archive: $EXPECTED_BR_TAR"
    FAILED=$((FAILED + 1))
fi

echo ""
echo "================================================================="
if [ "$FAILED" -eq 0 ]; then
    echo "  Host check PASSED (Errors: 0, Warnings: $WARNINGS)"
    echo "  Environment is ready for Phase 1."
    echo "================================================================="
    exit 0
else
    echo "  Host check FAILED (Errors: $FAILED, Warnings: $WARNINGS)"
    echo "  Please resolve required dependencies before proceeding."
    echo "================================================================="
    exit 1
fi
