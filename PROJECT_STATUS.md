# Project Status: Edge ML Linux OS (Track A)

**Target Hardware:** Raspberry Pi 4 Model B (4GB RAM)  
**Architecture:** ARM64 (`aarch64` / Cortex-A72 with NEON)  
**Emulation Target:** QEMU ARM64 Virt Machine (`aarch64`)  
**Build System:** Buildroot 2025.02.10 LTS  
**Primary Workload:** CPU-based MobileNetV2 INT8 Quantized Inference  
**Last Updated:** November 2026  

---

## 1. Current Phase
**Phase 2: Buildroot Configuration and Target Preparation**  
**Status:** COMPLETED — Pending User Approval to advance to Phase 3.

---

## 2. Completed Steps

### Phase 0: Workspace Inspection, Environment Verification & Architectural Foundation (Approved)
1. Inspected workspace, host environment (Ubuntu 24.04.4 LTS on WSL2, 20 vCPUs, 7.6GB RAM, 951GB free disk).
2. Verified pinned Buildroot 2025.02.10 tarball checksum (`f3de928ed368ef3dcadb97d4f4c1a9f49424a367759dba1d97c4c2bc5a22eab1`).
3. Initialized Git repository on branch `main` (`1d06c8c`), created `.gitignore`, `.gitkeep` markers, and `scripts/check_host_env.sh`.

### Phase 1: Requirements Analysis & Architectural Specification (Approved)
1. Completed comprehensive requirements specification in `docs/requirements.md` (hardware, boot latency targets, memory footprint, why stock Linux is unsuitable, build system, kernel, TFLite/LiteRT, security, and benchmarking).
2. Completed system architecture blueprint in `docs/architecture.md` with 5 Mermaid diagrams covering system layers, boot flow, storage/OverlayFS, ML pipeline, and security privilege dropping.
3. Committed changes to Git (`f5e0975`).

### Phase 2: Buildroot Configuration and Target Preparation (Completed)
1. **Archive Inspection & Verification:**
   - Re-verified SHA256 checksum of `buildroot/buildroot-2025.02.10.tar.gz` against official pinned hash (`f3de928ed368ef3dcadb97d4f4c1a9f49424a367759dba1d97c4c2bc5a22eab1`).
   - Inspected source archive contents for upstream `raspberrypi4_64_defconfig` and `qemu_aarch64_virt_defconfig`.
2. **Extraction of Pinned Buildroot Source:**
   - Extracted `buildroot-2025.02.10.tar.gz` into `buildroot/buildroot-2025.02.10/`.
   - Verified that gitignore properly ignores the extracted source tree to prevent repository bloat.
3. **Target A Configuration (Raspberry Pi 4 Model B, 4GB):**
   - Created customized defconfig: `configs/raspberrypi4_64_edgeml_defconfig`.
   - Architecture: ARM64 (`BR2_aarch64=y`, `BR2_cortex_a72=y`, `BR2_ARM_FPU_VFPV4=y`).
   - Toolchain: Buildroot internal toolchain, `glibc` C library (`BR2_TOOLCHAIN_BUILDROOT_GLIBC=y`), GCC 13.x (`BR2_GCC_VERSION_13_X=y`, 13.4.0), C++17 support enabled (`BR2_TOOLCHAIN_BUILDROOT_CXX=y`), thread support.
   - Init System: BusyBox (`BR2_INIT_BUSYBOX=y`).
   - Hardware Support: Raspberry Pi firmware for Pi 4 (`BR2_PACKAGE_RPI_FIRMWARE_VARIANT_PI4=y`), kernel 6.6 LTS branch.
4. **Target B Configuration (Generic ARM64 QEMU Virt Machine):**
   - Created dedicated testing defconfig: `configs/qemu_aarch64_virt_edgeml_defconfig`.
   - Architecture: ARM64 (`BR2_aarch64=y`), `glibc`, GCC 13.x, C++ support, BusyBox init.
   - Provides rapid local testing capability without physical SD card flashing.
5. **Build Automation & Verification Tooling:**
   - Created build helper script `scripts/build.sh` supporting `configure`, `check-config`, `menuconfig`, `savedefconfig`, and `clean` commands for both targets.
   - Configured out-of-tree builds (`output/` for RPi4, `output_qemu/` for QEMU virt).
   - Executed configuration verification checks for both targets.
   - Confirmed no kernel compilation or ML runtime builds were triggered.

---

## 3. Files Created or Modified

| File | Status | Description |
| :--- | :--- | :--- |
| `.gitignore` | Modified | Added `output_qemu/*` ignore rules |
| `output_qemu/.gitkeep` | Created | Preserves QEMU output folder in Git |
| `configs/raspberrypi4_64_edgeml_defconfig` | Created | Pinned defconfig for Raspberry Pi 4 Model B (4GB, ARM64, glibc, BusyBox) |
| `configs/qemu_aarch64_virt_edgeml_defconfig` | Created | Pinned defconfig for QEMU ARM64 Virt Emulation (glibc, BusyBox) |
| `scripts/build.sh` | Created | Build helper script for configuring, checking, and managing out-of-tree targets |
| `PROJECT_STATUS.md` | Modified | Updated with Phase 2 details, execution output, and next steps |

---

## 4. Commands Executed

```bash
# 1. Inspect archive and verify SHA256
tar -tzf buildroot/buildroot-2025.02.10.tar.gz | grep -E 'raspberrypi4_64_defconfig|qemu_aarch64_virt_defconfig'
sha256sum buildroot/buildroot-2025.02.10.tar.gz

# 2. Extract pinned Buildroot archive
tar -xzf buildroot/buildroot-2025.02.10.tar.gz -C buildroot/

# 3. Test & verify RPi4 configuration out-of-tree
chmod +x scripts/build.sh
./scripts/build.sh configure rpi4
./scripts/build.sh check-config rpi4

# 4. Test & verify QEMU virt configuration out-of-tree
./scripts/build.sh configure qemu
./scripts/build.sh check-config qemu
```

---

## 5. Build / Test Results

### 1. Verification of Target A (Raspberry Pi 4 Model B, 4GB):
```
>>> Configuring for: Raspberry Pi 4 Model B (4GB, ARM64)
>>> Using defconfig: /home/gouth/OS_Project_Track_A/configs/raspberrypi4_64_edgeml_defconfig
>>> Output dir:     /home/gouth/OS_Project_Track_A/output
# configuration written to /home/gouth/OS_Project_Track_A/output/.config

=================================================================
  Verifying Configuration: Raspberry Pi 4 Model B (4GB, ARM64)
=================================================================
BR2_ARCH="aarch64"
BR2_cortex_a72=y
BR2_TOOLCHAIN_BUILDROOT_GLIBC=y
BR2_GCC_VERSION="13.4.0"
BR2_TOOLCHAIN_BUILDROOT_CXX=y
BR2_INIT_BUSYBOX=y
=================================================================
```

### 2. Verification of Target B (QEMU ARM64 Virt Emulation):
```
>>> Configuring for: QEMU ARM64 Virt Emulation Target
>>> Using defconfig: /home/gouth/OS_Project_Track_A/configs/qemu_aarch64_virt_edgeml_defconfig
>>> Output dir:     /home/gouth/OS_Project_Track_A/output_qemu
# configuration written to /home/gouth/OS_Project_Track_A/output_qemu/.config

=================================================================
  Verifying Configuration: QEMU ARM64 Virt Emulation Target
=================================================================
BR2_ARCH="aarch64"
BR2_TOOLCHAIN_BUILDROOT_GLIBC=y
BR2_GCC_VERSION="13.4.0"
BR2_TOOLCHAIN_BUILDROOT_CXX=y
BR2_INIT_BUSYBOX=y
=================================================================
```

---

## 6. Errors and Their Resolutions

- None encountered. Both configurations resolved cleanly with glibc, AArch64, BusyBox, and GCC 13.4.0 C++17 support verified.

---

## 7. Remaining Tasks

- [ ] **Phase 3:** Linux Kernel Configuration & Storage/OverlayFS Setup (Strip BCM2711 kernel config, enable OverlayFS, SquashFS, performance governor, and 4KB pages).
- [ ] **Phase 4:** Root Filesystem Hardening & Security (BusyBox init scripts, read-only rootfs mount, `/etc` OverlayFS mount, unprivileged user `edgeai`, service script `S99edge-inference`).
- [ ] **Phase 5:** Machine Learning Stack Integration (Enable Buildroot `tensorflow-lite` package with XNNPACK and NEON optimizations, add MobileNetV2 INT8 quantized model and label assets).
- [ ] **Phase 6:** Edge Inference Application (Develop native C++ inference engine `edge-classify`, cross-compile and package).
- [ ] **Phase 7:** Full System Image Build & Genimage Integration (`sdcard.img` output generation).
- [ ] **Phase 8:** Target Deployment, Verification & Comparative Benchmarking (Collect empirical physical metrics against Raspberry Pi OS Lite).

---

## 8. Exact Next Step

**Awaiting user approval to proceed to Phase 3:**  
Configure the Linux kernel fragment for the BCM2711 SoC, enabling `CONFIG_OVERLAY_FS`, `CONFIG_SQUASHFS`, `CONFIG_EXT4_FS`, `CONFIG_ARM64_4K_PAGES`, and the performance CPU frequency governor while disabling unused audio, wireless, and legacy media drivers.
