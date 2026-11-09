# Project Status: Edge ML Linux OS (Track A)

**Target Hardware:** Raspberry Pi 4 Model B (4GB RAM)  
**Emulation Target:** QEMU ARM64 Virt Machine (`aarch64`)  
**Build System:** Buildroot 2025.02.10 LTS  
**Primary Workload:** CPU-based MobileNetV2 INT8 Quantized Inference  
**Last Updated:** November 2026  

---

## 1. Current Phase
**Phase Complete:** Git Reproducibility, Build Manifests, and Baseline Evaluation (Phase 1–3 & Verification)  
**Next Phase:** Phase 8 (Target A Hardware Build: Raspberry Pi 4 Model B & Comparative Benchmarking).

---

## 2. Completed Steps

### Phase 0: Workspace Inspection, Environment Verification & Architectural Foundation (Approved)
1. Inspected workspace, host environment (Ubuntu 24.04.4 LTS on WSL2, 20 vCPUs, 7.6GB RAM, 951GB free disk).
2. Verified pinned Buildroot 2025.02.10 tarball checksum (`f3de928ed368ef3dcadb97d4f4c1a9f49424a367759dba1d97c4c2bc5a22eab1`).
3. Initialized Git repository on branch `main` (`1d06c8c`), created `.gitignore`, `.gitkeep` markers, and `scripts/check_host_env.sh`.

### Phase 1: Requirements Analysis & Architectural Specification (Approved & Verified)
1. Completed comprehensive requirements specification in `docs/requirements.md` (hardware, boot latency targets, memory footprint, stock Linux deficiencies, build system, kernel, TFLite/LiteRT, security, and benchmarking).
2. Completed system architecture blueprint in `docs/architecture.md` with 5 Mermaid diagrams covering system layers, boot flow, storage/OverlayFS, ML pipeline, and security privilege dropping.
3. Verified all Phase 1 requirements, added Raspberry Pi 4B selection justification (Section 3.1), explicit performance target thresholds (Section 4.1/4.2), and automated startup / classification criteria (Section 7.4).

### Phase 2: Buildroot Configuration and Target Preparation (Approved)
1. Extracted pinned Buildroot 2025.02.10 archive into `buildroot/buildroot-2025.02.10/`.
2. Created `configs/raspberrypi4_64_edgeml_defconfig` (RPi 4B, 4GB, ARM64 Cortex-A72, glibc, GCC 13.4.0 C++17, BusyBox).
3. Created `configs/qemu_aarch64_virt_edgeml_defconfig` (QEMU ARM64 virt emulation, glibc, GCC 13.4.0 C++17, BusyBox).
4. Created build automation script `scripts/build.sh` and verified both defconfigs.

### Phase 3: Build Initial ARM64 Linux System for QEMU (Approved)
1. Diagnosed and resolved Buildroot error rejecting spaces in `$PATH` caused by WSL importing Windows paths.
2. Removed `host-qemu` from defconfig to leverage host QEMU 8.2.2.
3. Cross-compiled kernel 6.12.27, glibc 2.41, BusyBox 1.37.0, and rootfs image.
4. Verified clean boot to `buildroot login:` in QEMU virt machine.

### Phase 4: Root Filesystem Hardening & Storage/OverlayFS Setup (Approved)
1. Configured root filesystem to mount read-only (`ro`) in `rootfs/etc/fstab` and `inittab`.
2. Mounted volatile temporary filesystems as `tmpfs` on `/tmp` and `/run`.
3. Implemented `/etc/init.d/S00overlay` to create an ephemeral OverlayFS upper layer for `/etc` backed by `tmpfs`.
4. Hardened kernel parameters via `/etc/sysctl.d/99-security.conf` and created unprivileged system user `edgeai` (UID 1001).

### Phase 5: Machine Learning Runtime Integration (Approved)
1. Enabled TensorFlow Lite 2.11.0 C++ runtime in Buildroot with ARM NEON acceleration (Ruy GEMM matrix library).
2. Integrated INT8 quantized MobileNetV2 model and ImageNet labels in `/usr/share/edgeml/models/`.
3. Verified inference execution and memory footprint.

### Phase 6: Edge Inference Application (Approved)
1. Developed production C++ classification application [`/usr/bin/edge-classify`](src/edge_classify.cpp) with zero external image library dependencies (custom PPM P3/P6 loader and bilinear resizer).
2. Supported multi-threading (`--threads N`), top-K output (`--top K`), and high-precision benchmark loops (`--benchmark N`).
3. Verified execution against corrupt images, dimension mismatches, and multi-thread configurations.

### Phase 7: Edge ML System Integration & Evaluation (Approved)
1. Integrated boot-time service script `/etc/init.d/S99edgeml` with `/etc/default/edgeml` configuration.
2. Automated 15-test verification suite via `scripts/phase7_verify.sh` achieving 15/15 PASS.
3. Created release deliverables (`deliverables/images/`, `deliverables/config/`, `deliverables/package-manifest.json`).

### Phase 8 Pre-requisites: Baseline Evaluation & Reproducibility Verification (Complete)
1. **Stock Linux Baseline Evaluation:** Evaluated Debian GNU/Linux 13 (Trixie v13.7) running in matching QEMU ARM64 Cortex-A53 2-core setup.
2. **Comparative Benchmark Report:** Updated `docs/benchmarks.md` with side-by-side empirical metrics, methodology notes, and verified reductions:
   - **RootFS Content Reduction:** **99.10%** (2,414 MB vs 21.60 MB `du -sh`).
   - **Storage Image Footprint Reduction:** **98.12%** (3,825.4 MB vs 71.86 MB).
   - **System Idle RAM Reduction:** **91.67%** (156 MB vs 13 MB `free -m`).
   - **Package Count Reduction:** **94.31%** (404 packages vs 23 built packages).
   - **Boot Latency:** **37.33% faster** (26.76 s vs 16.77 s).
   - **Dual-Thread Saturation Throughput:** **3.98 FPS vs 3.85 FPS** (Custom OS faster with 10.34% less tail jitter).
3. **Reproducibility Verification:**
   - Application bit-for-bit determinism verified: Rebuilt `edge-classify` matched target binary SHA-256 (`e04c77b7f240167d0de34259fa58626a4013c10b60876364a2a7fab6c971f03e`).
   - Kernel release deliverable matched target image SHA-256 (`493da3b0d1a85549947f334f8419c2517872dc7dd4394e2ad015cedc768717e0`).
   - Out-of-tree defconfig regeneration proved 100% Kconfig option identity.
   - Build harness executed cleanly in empty environment (`env -i`).
   - Generated `deliverables/build_manifest.md`, `deliverables/SHA256SUMS.txt`, `SHA256SUMS.txt`, and `docs/reproducibility.md`.

---

## 3. Remaining Tasks

- [x] **Phase 0:** Workspace Inspection, Environment Verification & Architectural Foundation (COMPLETED & VERIFIED)
- [x] **Phase 1:** Requirements Analysis & Architectural Specification (COMPLETED & VERIFIED)
- [x] **Phase 2:** Buildroot Configuration and Target Preparation (COMPLETED & VERIFIED)
- [x] **Phase 3:** Build Initial ARM64 Linux System for QEMU (COMPLETED & VERIFIED)
- [x] **Phase 4:** Root Filesystem Hardening & Storage/OverlayFS Setup (COMPLETED & VERIFIED)
- [x] **Phase 5:** Machine Learning Runtime Integration (COMPLETED & VERIFIED)
- [x] **Phase 6:** Edge Inference Application (COMPLETED & VERIFIED)
- [x] **Phase 7:** Edge ML System Integration & Evaluation (COMPLETED & VERIFIED)
- [x] **Baseline Evaluation & Reproducibility:** Stock Linux Benchmarks & Build Manifests (COMPLETED & VERIFIED)
- [ ] **Phase 8:** Target A Hardware Build (Build full Raspberry Pi 4 Model B kernel 6.6, BCM2711 DTB, firmware, and generate `sdcard.img`) & Physical Comparative Benchmarking against Raspberry Pi OS Lite.

---

## 4. Exact Next Step

**Awaiting user approval to commit verified manifests and proceed to Phase 8:**  
Configure and build Target A for physical hardware: Raspberry Pi 4 Model B (4GB RAM) using `configs/raspberrypi4_64_edgeml_defconfig` (Linux kernel 6.6.x, Broadcom BCM2711 device tree, GPU firmware, bootloader config, and bootable `sdcard.img`), and perform comparative benchmarking against standard Raspberry Pi OS Lite.
