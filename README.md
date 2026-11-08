# Edge ML Linux OS (Track A)

> **Application-Specific Linux Distribution for ML Inference at the Edge**  
> Target: Raspberry Pi 4 Model B (4GB RAM) | Architecture: ARM64 (aarch64)

---

## 1. Project Overview

This repository contains the complete source code, configuration files, build manifests, and documentation for **Track A** of the Edge ML Linux OS project. 

The goal is to design and build an application-specific Embedded Linux operating system optimized specifically for high-efficiency INT8 machine learning inference at the edge, comparing its performance, resource utilization, boot latency, and security against standard Raspberry Pi OS Lite.

### Stack Specifications
- **Target Hardware:** Raspberry Pi 4 Model B (4GB RAM, Broadcom BCM2711)
- **Host Architecture:** Linux x86_64 (Ubuntu 24.04 LTS recommended)
- **Target Architecture:** `aarch64` (ARM64, Cortex-A72 with NEON)
- **Build System:** [Buildroot](https://buildroot.org/) (Pinned release: `2025.02.10 LTS`)
- **C Library:** GNU C Library (`glibc`)
- **Init System:** BusyBox `init` (`/sbin/init`)
- **Kernel:** Linux Kernel 6.6.y (Raspberry Pi LTS tree)
- **ML Framework:** TensorFlow Lite / LiteRT C++ runtime with XNNPACK delegate
- **Model:** MobileNetV2 INT8 Quantized (`mobilenet_v2_1.0_224_quant.tflite`)
- **Security:** Read-only root filesystem (`ro`), volatile `tmpfs` mounts, `overlayfs` for mutable layers, unprivileged application service user (`edgeai`).

---

## 2. Directory Structure

```
OS_Project_Track_A/
├── README.md               # Main project documentation and quickstart
├── PROJECT_STATUS.md       # Real-time phase tracking and development logs
├── .gitignore              # Build and workspace ignore rules
├── docs/                   # Formal architectural and design specifications
│   ├── requirements.md     # Detailed system & functional requirements
│   ├── architecture.md     # System architecture, boot sequence, security model
│   └── benchmark_plan.md   # Rigorous evaluation plan vs Raspberry Pi OS Lite
├── buildroot/              # Pinned Buildroot source archive and build tree
│   └── buildroot-2025.02.10.tar.gz  # Pinned release archive
├── configs/                # Defconfigs for Buildroot, Kernel, and Busybox
│   └── raspberrypi4_64_edgeml_defconfig
├── rootfs/                 # Root filesystem overlays (etc, inittab, init.d, users)
│   ├── etc/
│   │   ├── inittab
│   │   ├── init.d/
│   │   │   ├── rcS
│   │   │   ├── S01mountfs
│   │   │   ├── S10overlay
│   │   │   └── S99edge-inference
│   │   └── fstab
│   └── usr/
├── ml_model/               # Quantized model files, labels, and test samples
├── scripts/                # Automation, host check, flashing, and benchmark scripts
│   ├── check_host_env.sh   # Verifies host packages, disk, and archive hashes
│   ├── build.sh            # Reproducible build invocation script
│   └── run_benchmarks.sh   # Automated benchmark data collection script
├── tests/                  # Automated verification and host-side regression tests
└── output/                 # Build outputs, images, and benchmark reports (gitignored)
```

---

## 3. Host System Prerequisites

The build host must be running Linux (WSL2 Ubuntu 24.04 or native Linux x86_64).

Run the automated host environment verification tool:
```bash
./scripts/check_host_env.sh
```

Required packages on Ubuntu / Debian:
```bash
sudo apt-get update && sudo apt-get install -y \
    build-essential gcc g++ git make binutils patch \
    bzip2 tar gzip cpio unzip rsync file bc wget python3 \
    pkg-config libncurses-dev flex bison
```

---

## 4. Building the Operating System

Building is performed in phased, reproducible steps:

1. **Verify Host Environment:**
   ```bash
   ./scripts/check_host_env.sh
   ```

2. **Phase 1 — Extract and Configure Buildroot:**
   *(Follow instructions in PROJECT_STATUS.md upon Phase 1 kickoff)*

3. **Phase 2 — Cross-Compilation & RootFS Generation:**
   ```bash
   make -C buildroot/buildroot-2025.02.10 O=$(pwd)/output
   ```

The final output will be an SD card image at `output/images/sdcard.img`.

---

## 5. Security & Isolation Model

- **Read-Only Root Filesystem:** Flash wear is eliminated; unauthorized modifications to `/` are rejected at the kernel VFS layer.
- **OverlayFS:** Volatile changes are redirected to RAM (`tmpfs`), preventing corruption during sudden power loss.
- **Unprivileged Service:** Machine learning inference is executed strictly under user `edgeai` (`UID 1001`), with no superuser or network access.

---

## 6. Progress Tracking

Consult [PROJECT_STATUS.md](PROJECT_STATUS.md) for the active development phase, build logs, executed commands, test results, and immediate next steps.
