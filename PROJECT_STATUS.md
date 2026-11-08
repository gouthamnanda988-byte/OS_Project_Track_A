# Project Status: Edge ML Linux OS (Track A)

**Target Hardware:** Raspberry Pi 4 Model B (4GB RAM)  
**Emulation Target:** QEMU ARM64 Virt Machine (`aarch64`)  
**Build System:** Buildroot 2025.02.10 LTS  
**Primary Workload:** CPU-based MobileNetV2 INT8 Quantized Inference  
**Last Updated:** November 2026  

---

## 1. Current Phase
**Phase 3: Build Initial ARM64 Linux System for QEMU**  
**Status:** COMPLETED & VERIFIED — Pending User Approval to advance to Phase 4.

---

## 2. Completed Steps

### Phase 0: Workspace Inspection, Environment Verification & Architectural Foundation (Approved)
1. Inspected workspace, host environment (Ubuntu 24.04.4 LTS on WSL2, 20 vCPUs, 7.6GB RAM, 951GB free disk).
2. Verified pinned Buildroot 2025.02.10 tarball checksum (`f3de928ed368ef3dcadb97d4f4c1a9f49424a367759dba1d97c4c2bc5a22eab1`).
3. Initialized Git repository on branch `main` (`1d06c8c`), created `.gitignore`, `.gitkeep` markers, and `scripts/check_host_env.sh`.

### Phase 1: Requirements Analysis & Architectural Specification (Approved)
1. Completed comprehensive requirements specification in `docs/requirements.md` (hardware, boot latency targets, memory footprint, stock Linux deficiencies, build system, kernel, TFLite/LiteRT, security, and benchmarking).
2. Completed system architecture blueprint in `docs/architecture.md` with 5 Mermaid diagrams covering system layers, boot flow, storage/OverlayFS, ML pipeline, and security privilege dropping.
3. Committed changes to Git (`f5e0975`).

### Phase 2: Buildroot Configuration and Target Preparation (Approved)
1. Extracted pinned Buildroot 2025.02.10 archive into `buildroot/buildroot-2025.02.10/`.
2. Created `configs/raspberrypi4_64_edgeml_defconfig` (RPi 4B, 4GB, ARM64 Cortex-A72, glibc, GCC 13.4.0 C++17, BusyBox).
3. Created `configs/qemu_aarch64_virt_edgeml_defconfig` (QEMU ARM64 virt emulation, glibc, GCC 13.4.0 C++17, BusyBox).
4. Created build automation script `scripts/build.sh` and verified both defconfigs.
5. Committed changes to Git (`d891a90`).

### Phase 3: Build Initial ARM64 Linux System for QEMU (Completed & Verified)
1. **WSL Environment Sanitization:**
   - Diagnosed and resolved Buildroot error rejecting spaces in `$PATH` caused by WSL importing Windows paths (`/mnt/c/Program Files/...`).
   - Implemented automatic `$PATH` sanitization in `scripts/build.sh`.
2. **Defconfig Optimization:**
   - Removed `host-qemu` package compilation from `configs/qemu_aarch64_virt_edgeml_defconfig` to utilize host's pre-installed `qemu-system-aarch64 8.2.2`, avoiding unnecessary 25+ minute host toolchain overhead.
3. **Out-of-Tree Cross-Compilation:**
   - Built the QEMU ARM64 system in `output_qemu/` using a conservative `-j4` job limit suitable for 7.6GB RAM.
   - Cross-compiled:
     - GNU C Library (`glibc 2.40`)
     - Cross-GCC 13.4.0 (with C++ support) & Binutils 2.43.1
     - BusyBox 1.37.0 (`/sbin/init` and userland applets)
     - Linux Kernel 6.12.27 ARM64 (`output_qemu/images/Image`)
     - Root filesystem image (`output_qemu/images/rootfs.ext4`)
4. **Boot Verification in QEMU:**
   - Launched the generated kernel and rootfs in QEMU emulator (`qemu-system-aarch64 -M virt -cpu cortex-a53 ...`).
   - Confirmed 100% clean boot: MMU initialized, serial UART `ttyAMA0` operational, ext4 rootfs mounted, BusyBox `/sbin/init` started services, udhcpc acquired IP address `10.0.2.15`, and system reached `buildroot login:`.
5. **Automation Enhancement:**
   - Added `./scripts/build.sh run-qemu` command to launch the emulation directly with one command.

---

## 3. Files Created or Modified

| File | Status | Description |
| :--- | :--- | :--- |
| `configs/qemu_aarch64_virt_edgeml_defconfig` | Modified | Streamlined defconfig leveraging host QEMU 8.2.2 |
| `scripts/build.sh` | Modified | Added PATH sanitization, `build`, and `run-qemu` helper commands |
| `PROJECT_STATUS.md` | Modified | Updated with Phase 3 build output, artifacts, and verification logs |

---

## 4. Commands Executed

```bash
# 1. Update build script with PATH sanitization and build target
# 2. Configure QEMU ARM64 Virt target
./scripts/build.sh configure qemu

# 3. Cross-compile operating system with conservative 4-job parallelism
./scripts/build.sh build qemu 4

# 4. Inspect generated images
ls -lh output_qemu/images/
file output_qemu/images/*

# 5. Boot and verify operating system in QEMU emulator
./scripts/build.sh run-qemu
```

---

## 5. Build / Test Results

### 1. Generated Build Artifacts (`output_qemu/images/`)
- **Kernel Binary:** `output_qemu/images/Image` (Size: **12 MB**)  
  *Format:* Linux kernel ARM64 boot executable Image, little-endian, 4K pages.  
  *Kernel Version:* **Linux 6.12.27**.
- **Root Filesystem:** `output_qemu/images/rootfs.ext4` -> `rootfs.ext2` (Size: **60 MB**)  
  *Format:* Linux rev 1.0 ext4 filesystem data, volume name "rootfs".  
  *Init System:* **BusyBox 1.37.0** (`/sbin/init`).  
  *C Library:* **`glibc 2.40`** (AArch64).

### 2. Live QEMU Boot Verification Log (Extract)
```
Booting Linux on physical CPU 0x0000000000 [0x410fd034]
Linux version 6.12.27 (gouth@LAPTOP-UB3KGB9A) (aarch64-buildroot-linux-gnu-gcc 13.4.0) #1 SMP
Machine model: linux,dummy-virt
Memory: 113396K/131072K available
Serial: AMBA PL011 UART driver
9000000.pl011: ttyAMA0 at MMIO 0x9000000 (irq = 13, base_baud = 0) is a PL011 rev1
printk: legacy console [ttyAMA0] enabled
virtio_blk virtio0: [vda] 122880 512-byte logical blocks (62.9 MB/60.0 MiB)
EXT4-fs (vda): mounted filesystem a5330293-7d83-4976-9e41-089e35b8540b ro with ordered data mode.
VFS: Mounted root (ext4 filesystem) readonly on device 254:0.
Run /sbin/init as init process
Saving 256 bits of creditable seed for next boot
Starting syslogd: OK
Starting klogd: OK
Running sysctl: OK
Starting network: udhcpc: started, v1.37.0
udhcpc: lease of 10.0.2.15 obtained from 10.0.2.2, lease time 86400
OK
Starting crond: OK

Welcome to Buildroot
buildroot login:
```

---

## 6. Errors and Their Resolutions

1. **Issue:** Buildroot dependency check aborted with:
   `Your PATH contains spaces, TABs, and/or newline (\n) characters. This doesn't work. Fix your PATH environment variable.`  
   **Root Cause:** WSL2 automatically appends the host Windows system `%PATH%` (which contains directory names with spaces such as `/mnt/c/Program Files/...`) to the Linux `$PATH`.  
   **Resolution:** Added automatic `$PATH` sanitization logic at the entry point of `scripts/build.sh` to filter out any directory paths containing whitespace before invoking Buildroot.

---

## 7. Remaining Tasks

- [ ] **Phase 4:** Root Filesystem Hardening & Storage/OverlayFS Setup (Implement read-only rootfs mount, `/etc` OverlayFS mount, unprivileged user `edgeai` UID 1001, BusyBox init scripts `rcS`, `S01mountfs`, `S10overlay`).
- [ ] **Phase 5:** Machine Learning Runtime Integration (Enable Buildroot `tensorflow-lite` C++ package with XNNPACK and NEON optimizations, package MobileNetV2 INT8 model and labels).
- [ ] **Phase 6:** Edge Inference Application (Develop native C++ inference engine `edge-classify`, cross-compile and stage).
- [ ] **Phase 7:** Target A Hardware Build (Build full Raspberry Pi 4 Model B kernel 6.6, BCM2711 DTB, firmware, and generate `sdcard.img`).
- [ ] **Phase 8:** Target Deployment, Verification & Comparative Benchmarking (Empirical benchmarking against Raspberry Pi OS Lite).

---

## 8. Exact Next Step

**Awaiting user approval to proceed to Phase 4:**  
Configure root filesystem hardening on our working ARM64 system: establish rootfs overlay scripts (`rootfs/etc/inittab`, `rootfs/etc/init.d/rcS`, `rootfs/etc/init.d/S10overlay`), configure unprivileged user `edgeai` (UID: 1001), and verify read-only rootfs with OverlayFS writeable layer in QEMU.
