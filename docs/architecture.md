# System Architecture: Track A — Edge ML Linux OS

**System:** Application-Specific Linux Distribution for ML Inference at the Edge  
**Target:** Raspberry Pi 4 Model B (Broadcom BCM2711, Quad-core Cortex-A72 @ 1.5GHz, 4GB RAM)  
**Date:** November 2026  
**Status:** Approved (Phase 0)

---

## 1. High-Level Architectural Diagram

```
+-------------------------------------------------------------------------+
|                  EDGE ML INFERENCE APPLICATION LAYER                    |
|                                                                         |
|  +--------------------+   +-----------------------+   +--------------+  |
|  | C++ Preprocessor   |-->| TensorFlow Lite C++   |-->| Classification|  |
|  | (stb_image/libjpeg)|   | Engine (XNNPACK/NEON) |   | Post-process |  |
|  +--------------------+   +-----------------------+   +--------------+  |
|               ^                       ^                                 |
|               |                       | MobileNetV2 INT8 (~3.4MB)       |
|       Input Frame / Image             +---------------------------      |
|                                                                         |
|  Executed by: Unprivileged User 'edgeai' (UID 1001, GID 1001)           |
+-------------------------------------------------------------------------+
                                    |
+-------------------------------------------------------------------------+
|                        USERSPACE & INIT LAYER                           |
|                                                                         |
|  BusyBox 1.36.x (/sbin/init)                                            |
|  +-- /etc/inittab                                                       |
|      +-- /etc/init.d/rcS                                                |
|          +-- S01mountfs (proc, sysfs, devtmpfs, tmpfs on /tmp, /run)   |
|          +-- S10overlay  (mounts overlayfs for /etc, /var if writeable) |
|          +-- S99edge-inference (drop priv to edgeai, start ML daemon)   |
|                                                                         |
|  C Library: GNU C Library (glibc 2.40+)                                 |
+-------------------------------------------------------------------------+
                                    |
+-------------------------------------------------------------------------+
|                    VFS & FILESYSTEM MOUNT ARCHITECTURE                  |
|                                                                         |
|  +-------------------+  +-----------------------+  +-----------------+  |
|  | / (Root Partition)|  | /tmp, /run, /var      |  | /data           |  |
|  | Read-Only ext4 /  |  | Volatile tmpfs        |  | Writeable ext4  |  |
|  | SquashFS (ro)     |  | (RAM-backed, no wear) |  | (logs/telemetry)|  |
|  +-------------------+  +-----------------------+  +-----------------+  |
|            \                        /                        /          |
|             \                      /                        /           |
|          OverlayFS: lowerdir=/etc (ro), upperdir=/data/etc (rw)         |
+-------------------------------------------------------------------------+
                                    |
+-------------------------------------------------------------------------+
|                         LINUX KERNEL LAYER                              |
|                                                                         |
|  Linux Kernel 6.6.y (ARM64 / AArch64)                                   |
|  - Broadcom BCM2711 SoC Drivers (Clocks, Power, UART, SDHCI, DMA)       |
|  - Memory Management: 4KB pages, slab allocator, cgroups                |
|  - Block Layer & Drivers: MMC/SD card driver, OverlayFS, SquashFS, Ext4 |
|  - In-tree Flattened Device Tree: bcm2711-rpi-4-b.dtb                   |
+-------------------------------------------------------------------------+
                                    |
+-------------------------------------------------------------------------+
|                     HARDWARE & FIRMWARE LAYER                           |
|                                                                         |
|  1. Broadcom BCM2711 On-Chip ROM (Loads bootloader EEPROM)              |
|  2. Raspberry Pi 4 EEPROM Bootloader                                    |
|  3. VideoCore VI Firmware: bootcode.bin, start4.elf                     |
|  4. Hardware: 4x Cortex-A72 @ 1.5GHz (ARMv8-A + NEON), 4GB LPDDR4-3200  |
+-------------------------------------------------------------------------+
```

---

## 2. Boot Flow Sequence & Timing Breakdown

1. **Stage 1 — SoC ROM:**
   - Power-on reset initializes the BCM2711. The primary on-chip boot ROM searches for the SPI EEPROM containing the stage-2 bootloader.
2. **Stage 2 — SPI EEPROM:**
   - The EEPROM bootloader initializes low-level hardware registers, LPDDR4 memory controllers, and the SD card controller (eMMC/SDHCI).
   - Reads the FAT32 boot partition for `start4.elf` and `config.txt`.
3. **Stage 3 — VideoCore Firmware (`start4.elf`):**
   - Reads `config.txt` parameters (e.g. `arm_64bit=1`, `enable_uart=1`, `gpu_mem=64`).
   - Loads the 64-bit Linux kernel image (`Image`) and Device Tree (`bcm2711-rpi-4-b.dtb`) into RAM.
   - Passes kernel command-line arguments from `cmdline.txt`.
   - Releases the ARM Cortex-A72 cores from reset at EL2/EL1.
4. **Stage 4 — Linux Kernel Initialization:**
   - Early architecture setup (MMU page tables, vector tables, interrupt controller GIC-400).
   - Driver probing: Serial (`ttyAMA0`), Storage (`mmc0`), Timer, DMA.
   - Mounts the root filesystem from `/dev/mmcblk0p2` in **read-only mode** (`ro`).
   - Spawns PID 1 (`/sbin/init`).
5. **Stage 5 — BusyBox Init & Userspace Orchestration:**
   - `/sbin/init` reads `/etc/inittab`.
   - Executes `/etc/init.d/rcS` sysinit script:
     - Mounts essential virtual filesystems: `proc`, `sysfs`, `devtmpfs`.
     - Mounts volatile RAM filesystems: `tmpfs` on `/tmp`, `/run`, and `/var/run`.
     - Mounts `/data` (`/dev/mmcblk0p3`) if present.
     - Mounts `overlayfs` on designated mutable directories (`/etc`).
   - Invokes `/etc/init.d/S99edge-inference`:
     - Changes UID/GID to unprivileged user `edgeai` (`su -s /bin/sh edgeai -c ...`).
     - Launches the native C++ inference engine (`/usr/bin/edge-classify`).
6. **Stage 6 — ML Inference Pipeline:**
   - Pre-allocates model tensor memory buffers.
   - Attaches XNNPACK delegate for NEON vector acceleration.
   - Runs model inference on test images / camera stream.

---

## 3. Filesystem & Storage Architecture

### 3.1 Partition Scheme
The SD card is partitioned into three distinct segments:

| Partition | Label | Filesystem | Mount Point | Permissions | Content |
| :--- | :--- | :--- | :--- | :--- | :--- |
| `/dev/mmcblk0p1` | `BOOT` | FAT32 | `/boot` | `ro` | Firmware, Kernel `Image`, DTBs, `config.txt` |
| `/dev/mmcblk0p2` | `ROOTFS` | ext4 / squashfs | `/` | `ro` | Immutable OS, libraries, ML runtime, model |
| `/dev/mmcblk0p3` | `DATA` | ext4 | `/data` | `rw` | Persistent logs, benchmark metrics |

### 3.2 OverlayFS Implementation
To guarantee that root cannot be corrupted during unexpected power loss, the root filesystem remains mounted read-only at all times:
- `lowerdir`: `/etc` (read-only base image).
- `upperdir`: `/data/overlay/etc_upper` (or volatile `tmpfs/etc_upper`).
- `workdir`: `/data/overlay/etc_work` (or volatile `tmpfs/etc_work`).
- `merged`: `/etc`.

---

## 4. Security & Privilege Separation

1. **Unprivileged Execution:**
   - Service user: `edgeai` (UID: 1001, GID: 1001, Home: `/home/edgeai`, Shell: `/bin/false`).
   - The inference binary has no write permissions outside `/tmp` or `/data/logs`.
   - File permissions:
     - `/usr/bin/edge-classify`: `0755 root:root`
     - `/usr/share/models/mobilenet_v2_int8.tflite`: `0644 root:root`
     - `/data/logs`: `0775 edgeai:edgeai`
2. **Network Attack Surface:**
   - Production images have SSH, Telnet, and FTP disabled.
   - No listening sockets in default operation.
3. **Hardened Kernel Parameters:**
   - Kernel command line includes: `security=selinux/smack` (or standard DAC with restricted sysctl: `kernel.kptr_restrict=2`, `kernel.dmesg_restrict=1`).

---

## 5. Machine Learning Runtime Architecture

```
+--------------------------------------------------------------------------+
|                  edge-classify (C++17 Executable)                       |
|                                                                          |
|  +--------------------------------------------------------------------+  |
|  | Input Image Ingestion (224x224x3 RGB)                             |  |
|  | - Read binary bitmap or pre-processed tensor buffer                |  |
|  +--------------------------------------------------------------------+  |
|                                 |                                        |
|  +--------------------------------------------------------------------+  |
|  | TFLite Model Loader & FlatBuffer Verifier                          |  |
|  | - Loads /usr/share/models/mobilenet_v2_1.0_224_quant.tflite        |  |
|  +--------------------------------------------------------------------+  |
|                                 |                                        |
|  +--------------------------------------------------------------------+  |
|  | TFLite Interpreter + XNNPACK Delegate                              |  |
|  | - SetNumThreads(4)                                                 |  |
|  | - AllocateTensors()                                                |  |
|  | - ARM64 NEON INT8 Vectorized Kernel Execution                      |  |
|  +--------------------------------------------------------------------+  |
|                                 |                                        |
|  +--------------------------------------------------------------------+  |
|  | Post-Processing & Top-K Classification Output                      |  |
|  | - Softmax dequantization / Argmax selection                        |  |
|  | - High-resolution benchmark timestamps: clock_gettime(MONOTONIC)   |  |
|  +--------------------------------------------------------------------+  |
+--------------------------------------------------------------------------+
```

---

## 6. Buildroot Integration Strategy

Buildroot acts as the single orchestrator:
1. Toolchain generation: `aarch64-buildroot-linux-gnu` with `glibc`.
2. Packages:
   - `package/tensorflow-lite` (includes `flatbuffers`, `cpuinfo`, `farmhash`, `fft2d`, `gemmlowp`, `libabseil-cpp`, `ruy`, `xnnpack`).
   - Custom in-tree or external package for `edge-classify`.
3. RootFS post-build script:
   - Strips unneeded binaries, documentation, manpages, locales.
   - Sets up `/etc/inittab`, user `edgeai`, and permissions.
4. Genimage post-image script:
   - Generates the complete ready-to-flash `sdcard.img` containing `boot.vfat` and `rootfs.ext4`.
