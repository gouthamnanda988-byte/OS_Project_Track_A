# Project Status: Edge ML Linux OS (Track A)

**Target Hardware:** Raspberry Pi 4 Model B (4GB RAM)  
**Architecture:** ARM64 (`aarch64` / Cortex-A72 with NEON)  
**Build System:** Buildroot 2025.02.10 LTS  
**Primary Workload:** CPU-based MobileNetV2 INT8 Quantized Inference  
**Last Updated:** November 2026  

---

## 1. Current Phase
**Phase 1: Requirements Analysis & Architectural Specification**  
**Status:** COMPLETED — Pending User Approval to advance to Phase 2.

---

## 2. Completed Steps

### Phase 0: Workspace Inspection, Environment Verification & Architectural Foundation (Approved)
1. Inspected host workspace at `/home/gouth/OS_Project_Track_A`.
2. Verified host build environment (Ubuntu 24.04.4 LTS, 20 vCPUs, 7.6GB RAM, 951GB free disk, GNU Make 4.3, GCC 13.3.0, G++ 13.3.0, Git 2.43.0, Python 3.12.3).
3. Verified pinned Buildroot 2025.02.10 tarball checksum (`f3de928ed368ef3dcadb97d4f4c1a9f49424a367759dba1d97c4c2bc5a22eab1`).
4. Confirmed in-tree support for `raspberrypi4_64_defconfig` and `package/tensorflow-lite`.
5. Created version control configuration (`.gitignore`, `.gitkeep` markers) and initialized Git repository on branch `main` (commit `1d06c8c`).
6. Created and verified automated host environment audit script `scripts/check_host_env.sh`.

### Phase 1: Requirements Analysis & Architectural Specification (Completed)
1. **Requirements Specification Analysis:**
   - Authored formal engineering requirements in `docs/requirements.md` covering all Track A mandate areas.
   - Detailed why stock Linux (Raspberry Pi OS Lite, Debian, Ubuntu) is unsuitable for edge inference (15-30s boot latency, 160-220MB idle RAM bloat, 1.8-2.5GB storage bloat, vulnerability to flash corruption on power cut, non-deterministic scheduling jitter, and large attack surface).
   - Formulated explicit hardware requirements for Raspberry Pi 4 Model B (Broadcom BCM2711, 4x Cortex-A72 @ 1.5GHz with NEON SIMD, 4GB LPDDR4, UART0 `ttyAMA0`, 5V/3A USB-C power budget).
   - Defined clear proposed target metrics (Cold boot < 3.0s, Boot to 1st inference < 4.5s, Image size < 200MB, Idle RAM < 35MB, 4T inference ~15-22ms), strictly noting them as proposed targets rather than fabricated measurements.
   - Specified build system requirements (pinned Buildroot 2025.02.10 LTS, reproducible builds, out-of-tree structure, AArch64 glibc toolchain, GCC >= 12 with C++17).
   - Specified kernel and driver requirements (Linux 6.6.y, BCM2711 DTB, OverlayFS, SquashFS, ext4, 4KB pages, cgroups, performance CPU governor, stripping sound/wireless/unused multimedia).
   - Specified ML runtime requirements (TensorFlow Lite C++ with XNNPACK and NEON acceleration, MobileNetV2 INT8 quantized model, zero Python in userspace, native C++ `edge-classify` binary).
   - Specified security and privilege requirements (read-only rootfs, volatile tmpfs for `/tmp` and `/run`, OverlayFS for mutable `/etc`, unprivileged user `edgeai` UID/GID 1001, complete privilege dropping, no listening network daemons).
   - Specified deployment and comparative benchmarking protocol against Raspberry Pi OS Lite.
2. **System Architecture Specification:**
   - Authored architectural blueprint in `docs/architecture.md`.
   - Developed compliant Mermaid diagrams:
     - End-to-end 5-layer system block diagram (`flowchart TD`).
     - Cold boot flow and state progression flowchart (`flowchart TD`).
     - Storage partitioning and OverlayFS mechanics flowchart (`flowchart LR`).
     - Machine learning inference pipeline flowchart (`flowchart TD`).
     - Security boundary and privilege dropping sequence flowchart (`flowchart TD`).
   - Detailed the out-of-tree Buildroot directory structure and invocation scheme.

---

## 3. Files Created or Modified

| File | Status | Description |
| :--- | :--- | :--- |
| `docs/requirements.md` | Modified / Expanded | Comprehensive Track A requirements specification covering all assignment areas, stock Linux deficiencies, and target metrics |
| `docs/architecture.md` | Modified / Expanded | System architecture document with Mermaid diagrams covering layers, boot sequence, storage/OverlayFS, ML pipeline, and security |
| `PROJECT_STATUS.md` | Modified | Updated with Phase 1 execution details, command logs, and next steps |

---

## 4. Commands Executed

```bash
# Verify git status prior to Phase 1 implementation
wsl.exe -d Ubuntu -e bash -c "cd /home/gouth/OS_Project_Track_A && git status"

# Verification of documentation rendering and syntax
# (Document content written and inspected directly)
```

---

## 5. Build / Test Results

- Documentation syntax and Mermaid structural compliance verified.
- Pinned toolchain and target specifications matched against Buildroot 2025.02.10 capabilities.
- All performance figures clearly delineated as **Proposed Engineering Targets**.
- Zero downloads or builds performed in this phase per instructions.

---

## 6. Errors and Their Resolutions

- None encountered during Phase 1.

---

## 7. Remaining Tasks

- [ ] **Phase 2:** Base System & Toolchain Setup (Extract Buildroot 2025.02.10, configure base defconfig, verify cross-toolchain).
- [ ] **Phase 3:** Kernel & Storage Configuration (Configure minimal BCM2711 Linux 6.6 kernel with OverlayFS, SquashFS, and 4KB pages).
- [ ] **Phase 4:** Root Filesystem Hardening & Security (BusyBox init, read-only rootfs, `/etc` OverlayFS, unprivileged user `edgeai`, service script `S99edge-inference`).
- [ ] **Phase 5:** Machine Learning Stack Integration (Configure Buildroot `tensorflow-lite` package with XNNPACK and NEON optimizations, add MobileNetV2 INT8 quantized model and label assets).
- [ ] **Phase 6:** Edge Inference Application (Develop native C++ inference engine `edge-classify`, cross-compile and package).
- [ ] **Phase 7:** Full System Image Build & Genimage Integration (`sdcard.img` output generation).
- [ ] **Phase 8:** Target Deployment, Verification & Comparative Benchmarking (Collect empirical physical metrics against Raspberry Pi OS Lite).

---

## 8. Exact Next Step

**Awaiting user approval to proceed to Phase 2:**  
Extract `buildroot/buildroot-2025.02.10.tar.gz` into `buildroot/buildroot-2025.02.10/`, establish the base defconfig `configs/raspberrypi4_64_edgeml_defconfig` based on `raspberrypi4_64_defconfig`, configure the AArch64 glibc toolchain, and set up the out-of-tree build wrapper script `scripts/build.sh`.
