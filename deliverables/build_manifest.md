# Build Manifest: Edge ML Linux OS (Track A)

**Project:** Application-Specific Linux Distribution for ML Inference at the Edge (Track A)  
**Distribution Framework:** Buildroot 2025.02.10 LTS  
**Primary Target:** Raspberry Pi 4 Model B (Quad Cortex-A72 @ 1.5 GHz, 4GB RAM)  
**Emulation Verification Target:** QEMU ARM64 `virt` Machine (Cortex-A53, 2 vCPUs)  
**Generated Date:** November 2026  
**Document Revision:** 1.0 (Phase 8 Reproducibility Verification)

---

## 1. System & Architecture Overview

| Parameter | Specification | Verification Evidence |
| :--- | :--- | :--- |
| **Build System** | Buildroot 2025.02.10 LTS | `buildroot/buildroot-2025.02.10.tar.gz` (SHA-256: `f3de928ed368ef3dcadb97d4f4c1a9f49424a367759dba1d97c4c2bc5a22eab1`) |
| **Linux Kernel** | Long-Term Support (LTS) 6.12.27 | `downloads/linux/linux-6.12.27.tar.xz` (SHA-256: `8f4655a4cc7f93d72f515bbca54756de26ddaf5949790da6a17f766e3c33dc79`) |
| **Architecture** | AArch64 / ARM64 (64-bit Little Endian) | `BR2_aarch64=y` in `configs/qemu_aarch64_virt_edgeml_defconfig` |
| **Reference CPU** | ARM Cortex-A53 (Emulation) / Cortex-A72 (RPi4) | QEMU `-cpu cortex-a53` / BCM2711 SoC |
| **Init Subsystem** | BusyBox v1.37.0 SysV Init (`/sbin/init` $\to$ `/etc/init.d/rcS`) | `BR2_INIT_BUSYBOX=y` |
| **C Standard Library** | GNU C Library (`glibc`) v2.41 | `BR2_TOOLCHAIN_BUILDROOT_GLIBC=y` |
| **C++ Standard** | ISO C++17 (`-std=c++17`) | `BR2_TOOLCHAIN_BUILDROOT_CXX=y` |
| **Cross-Compiler** | GCC 13.4.0 (`aarch64-buildroot-linux-gnu-gcc`) | GNU Binutils 2.43.1, Linux 6.12 UAPI headers |

---

## 2. Configuration & Manifest Details

### 2.1 Configuration Files
* **QEMU Emulation Defconfig:** [`configs/qemu_aarch64_virt_edgeml_defconfig`](file:///home/gouth/OS_Project_Track_A/configs/qemu_aarch64_virt_edgeml_defconfig) (SHA-256: `5584698dc09ebfd53ec93c4a5447f993edd7329403bc4d5c8160f64b73d7a5bb`)
* **Raspberry Pi 4 Defconfig:** [`configs/raspberrypi4_64_edgeml_defconfig`](file:///home/gouth/OS_Project_Track_A/configs/raspberrypi4_64_edgeml_defconfig) (SHA-256: `e47d4772c7662c81490257d77106b705abc39cc422c0cfcbabd936da65ad4cb2`)
* **Unprivileged User Table:** [`configs/users_table.txt`](file:///home/gouth/OS_Project_Track_A/configs/users_table.txt) (SHA-256: `7324cb5c33f71d286664f4a900e9af0ca6fd94887df13c6e229f680177f7be00`)
  * Provisions `edgeai` unprivileged system user (`UID 1001`, `GID 1001`, `/home/edgeai`, login locked).
* **Package Manifest:** [`deliverables/package-manifest.json`](file:///home/gouth/OS_Project_Track_A/deliverables/package-manifest.json) (Buildroot `show-info` export detailing all 23 packages, upstream URLs, licenses, and patches).

---

## 3. Machine Learning Inference Stack

### 3.1 TensorFlow Lite Runtime & Acceleration
* **Runtime Version:** TensorFlow Lite 2.11.0 (C++ API)
* **Matrix Acceleration:** `Ruy` matrix multiplication library (tuned for ARM NEON SIMD)
* **Serialization:** FlatBuffers v24.12.23 (zero-copy model FlatBuffer parsing)
* **Core Dependencies:**
  * Google Abseil C++ (`libabseil-cpp`) v20250127.0
  * Eigen v3.4.0 (template linear algebra)
  * cpuinfo (Git revision `8a1772a0c5c447df2d18edf33ec4603a8c9c04a6`)
  * FarmHash (Git revision `0d859a811870d10f53a594927d0d0b97573ad06d`)
  * FFT2D (Fast Fourier Transform 2D)
  * gemmlowp (Low-precision GEMM)
  * neon-2-sse (ARM NEON compatibility headers)

### 3.2 Production Application Source Code
* **Binary Name:** `/usr/bin/edge-classify` (Stripped AArch64 executable, 82,784 bytes)
* **Application SHA-256:** `e04c77b7f240167d0de34259fa58626a4013c10b60876364a2a7fab6c971f03e`
* **Source Tree (`src/`):**
  * `src/edge_classify.cpp` (SHA-256: `9305115f8312c7469531d25d92837f3c7e7788ad70da79d53df6d87bc576ac23`)
  * `src/image_loader.cpp` (SHA-256: `0d8281086fbd195099da8c1422a248c9160fa3d4bfaeb6259e229506e515a2aa`)
  * `src/image_loader.h` (SHA-256: `d84310953100e5afa1d3dc628a8391b753d7ce77253edcbb96f950bfb28dbc04`)
  * `src/image_preprocessor.cpp` (SHA-256: `764613b6a091e05992eee9a40f228f052ea1d548bb63e24f2ff6b985c5a52694`)
  * `src/image_preprocessor.h` (SHA-256: `37858b73b66ac8300cc98e2fbed5fe188fed2ce77614021ba92106da796e2d9b`)
  * `src/Makefile` (SHA-256: `8c5efe547925cbd551a756eb858a98aa5d78be26ed88f4bd4bc2c247d9848b81`)

### 3.3 Model Artifacts
* **Model File:** `ml_model/mobilenet_v2_1.0_224_quant.tflite`
  * Size: 3,577,760 bytes (~3.41 MB)
  * Format: TensorFlow Lite FlatBuffer (INT8 Quantized, $1\times224\times224\times3$, 1001 classes)
  * SHA-256: `f08d447cde49b4e0446428aa921aff0a14ea589fa9c5817b31f83128e9a43c1d`
* **Labels File:** `ml_model/imagenet_labels.txt`
  * Size: 21,686 bytes (1001 ImageNet categories)
  * SHA-256: `b8aaeb5630c62626a7d124a05c0e14230a7ffdd2136a31d0190a4fb71c1d82f1`

---

## 4. Build Host Environment

* **Host Operating System:** Ubuntu 24.04.4 LTS (`noble`)
* **Host Kernel:** Linux `5.15.167.4-microsoft-standard-WSL2` (`x86_64`)
* **Host Platform:** WSL2 on Windows 11
* **Host Toolchain:**
  * GNU Make: `v4.3`
  * Host GCC: `v13.3.0`
  * GNU Binutils: `v2.42`
  * Python: `v3.12.3`
  * CMake: `v3.28.3`
* **PATH Sanitation:** `scripts/build.sh` filters all space-containing PATH entries imported from Windows to prevent Buildroot build system errors.
