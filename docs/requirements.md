# Requirements Specification: Track A — Edge ML Linux OS

**Project Name:** Application-Specific Linux Distribution for ML Inference at the Edge  
**Track:** Track A (Raspberry Pi 4 Model B — ARM64)  
**Date:** November 2026  
**Status:** Approved (Phase 0)

---

## 1. Executive Overview

The objective of this project is to engineer an application-specific, minimal, secure, and reproducible Embedded Linux operating system tailored specifically for Edge Machine Learning inference on the Raspberry Pi 4 Model B (4GB RAM). 

Unlike general-purpose distributions (such as Raspberry Pi OS Lite or Ubuntu Server), which carry generic daemons, package managers, and large Python runtime stacks, this distribution is stripped to the bare essentials required to execute INT8 quantized neural network inference with minimal cold-boot latency, a hardened read-only root filesystem, and low memory footprint.

---

## 2. Hardware Specification

| Parameter | Value |
| :--- | :--- |
| **Target Board** | Raspberry Pi 4 Model B (Revision 1.4 or 1.5) |
| **SoC** | Broadcom BCM2711 |
| **CPU** | Quad-core ARM Cortex-A72 (ARMv8-A 64-bit) @ 1.5 GHz (or 1.8 GHz) |
| **FPU / Vector Unit** | ARM NEON Advanced SIMD & VFPv4 |
| **System Memory** | 4 GB LPDDR4-3200 SDRAM |
| **Storage Medium** | Class 10 / UHS-I MicroSD Card (>= 16 GB) |
| **Primary I/O** | UART serial console (`ttyAMA0` / `serial0` @ 115200 baud), HDMI for console display |
| **Networking** | Gigabit Ethernet (BCM54213PE) / Optional Wi-Fi for metric egress |

---

## 3. Toolchain & Operating System Requirements

### 3.1 Build System & Toolchain
- **Build System:** Buildroot pinned to stable release **2025.02.10 LTS** (SHA256: `f3de928ed368ef3dcadb97d4f4c1a9f49424a367759dba1d97c4c2bc5a22eab1`).
- **Target Architecture:** `aarch64` (ARM64, little-endian).
- **Target CPU Variant:** `cortex-a72`.
- **C Library:** `glibc` (GNU C Library) — strictly required for TensorFlow Lite C++ runtime compatibility.
- **Compiler Version:** GNU Compiler Collection (GCC) >= 12.x with C++17 support and thread-local storage (`BR2_TOOLCHAIN_GCC_AT_LEAST_12=y`).
- **Binutils:** Pin to stable release (e.g. 2.41+).
- **Reproducibility:** Pinned hashes for all source archives and reproducible build timestamps (`BR2_REPRODUCIBLE=y`).

### 3.2 Kernel Specification
- **Kernel Tree:** Raspberry Pi vendor-supported Linux Kernel (`linux-rpi-6.6.y` or LTS compatible).
- **Kernel Defconfig:** Tailored `bcm2711_defconfig` stripped of unused drivers (sound, legacy multimedia, unused wireless chipsets).
- **Required Kernel Features:**
  - `CONFIG_OVERLAY_FS=y` (OverlayFS support for writeable layers over read-only root).
  - `CONFIG_SQUASHFS=y` and `CONFIG_SQUASHFS_XZ=y` / `CONFIG_EXT4_FS=y`.
  - `CONFIG_ARM64_4K_PAGES=y`.
  - `CONFIG_CGROUPS=y` and `CONFIG_NAMESPACES=y` (process isolation and resource monitoring).
  - `CONFIG_PRINTK_TIME=y` (kernel boot timestamping for latency profiling).

### 3.3 Init System & Userspace
- **Init Manager:** BusyBox `init` (`/sbin/init`) reading `/etc/inittab`.
- **Shell:** BusyBox `ash` (Bourne-compatible).
- **Device Management:** `mdev` or `devtmpfs` with auto-mounting.
- **Service Orchestration:** Lightweight SysV-style run-scripts located in `/etc/init.d/` (`S01mountfs`, `S10overlay`, `S99edge-inference`).
- **Eliminated Components:** `systemd`, `dbus`, `polkit`, `snapd`, `apt`, `cron`, `ssh` (disabled in release, optional debug image).

---

## 4. Storage & Filesystem Architecture

### 4.1 Partition Layout (SD Card)
1. **Partition 1 (`boot`, FAT32, ~64 MB):**
   - Raspberry Pi firmware (`bootcode.bin`, `start4.elf`, `fixup4.dat`).
   - `config.txt` and `cmdline.txt`.
   - Kernel binary `Image` and Device Tree Blob `bcm2711-rpi-4-b.dtb` + overlays.
2. **Partition 2 (`rootfs`, Read-Only SquashFS or ext4, ~150-250 MB):**
   - Immutable base system, libraries, and inference binary.
   - Mounted read-only (`ro` flag in `cmdline.txt` and `/etc/fstab`).
3. **Partition 3 (`data`, ext4, writeable, remaining SD space):**
   - Persistent configuration, calibration data, and inference logs.
   - Non-critical; corruption does not affect OS bootability.

### 4.2 OverlayFS Requirements
- `/tmp` and `/run` mounted as volatile `tmpfs`.
- `/var` redirected to `tmpfs` or overlaid on `/data/var` so ephemeral logging cannot wear out flash storage.
- Optional `/etc` upper layer on `tmpfs` or `/data/etc` for runtime adjustments.

---

## 5. Security & Isolation Specification

1. **Non-Root Execution:**
   - The ML inference application MUST run under an unprivileged system user (`edgeai`, UID 1001, GID 1001).
   - The `edgeai` user must not possess sudo privileges or root capabilities.
2. **Read-Only Enclosure:**
   - Write operations to the root filesystem partition must fail immediately at the VFS level (`EROFS`).
3. **Attack Surface Reduction:**
   - No open listening network ports in production configuration.
   - Password authentication disabled for root; direct console serial access only for field maintenance.

---

## 6. Machine Learning Inference Stack

### 6.1 Framework & Runtime
- **Runtime:** TensorFlow Lite (or LiteRT C++ / C API).
- **Execution Language:** Native C++ (compiled with `-O3 -mcpu=cortex-a72 -mfpu=neon-fp-armv8`).
- **Accelerators / Backend:**
  - `XNNPACK` delegate enabled for optimized ARM NEON vectorization.
  - Multi-threaded CPU execution (configurable 1, 2, 4 threads).
- **Zero Python Overhead:** No Python interpreter, NumPy, or bulky runtime packages on target image.

### 6.2 Model & Validation Data
- **Model Architecture:** MobileNetV2 (1.0x multiplier, 224x224 input resolution).
- **Quantization:** INT8 fully quantized weights and activations (TFLite FlatBuffer format, ~3.4 MB).
- **Input Pipeline:** Lightweight C/C++ image preprocessor (RGB raw parsing / stb_image or minimal OpenCV/libjpeg).
- **Test Dataset:** Standard ImageNet-compatible evaluation samples with label mapping (`labels.txt`).

---

## 7. Comparative Benchmark & Evaluation Requirements

The final custom operating system must be benchmarked against **Raspberry Pi OS Lite (64-bit)** running on identical hardware (RPi 4 Model B, 4GB RAM) using identical MicroSD cards.

### Key Performance Indicators (KPIs)

| Metric | Target (Custom Track A OS) | Baseline (RPi OS Lite) | Evaluation Method |
| :--- | :--- | :--- | :--- |
| **Boot Time (Cold Power to Init)** | < 3.5 seconds | ~15 - 25 seconds | Serial timestamps / oscilloscope probe |
| **Boot Time to 1st Inference** | < 5.0 seconds | ~30 - 45 seconds | Timestamped log in inference app |
| **RootFS Image Size** | < 250 MB | ~1.5 - 2.5 GB | `ls -lh` / image file size |
| **Idle RAM Consumption** | < 40 MB | ~150 - 250 MB | `free -m` / `/proc/meminfo` |
| **Inference Latency (INT8 1-thread)** | Measure p50, p95, p99 | Measure p50, p95, p99 | 100 iterations, high-res clock |
| **Inference Latency (INT8 4-threads)** | Measure p50, p95, p99 | Measure p50, p95, p99 | 100 iterations, high-res clock |
| **Process Memory Footprint (VSS/RSS)** | < 35 MB RSS | Measured against Python / C++ | `/proc/<pid>/smaps` or `ps -o rss` |
| **Filesystem Resiliency** | 100% survival after abrupt power cut | Vulnerable to dirty bit / fsck repair | Abrupt power cycle stress test (10x) |
