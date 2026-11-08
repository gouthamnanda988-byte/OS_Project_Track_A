# Formal Requirements Specification: Track A — Edge ML Linux OS

**Project Title:** Application-Specific Linux Distribution for ML Inference at the Edge  
**Track:** Track A (Target: Raspberry Pi 4 Model B, 4GB RAM — ARM64)  
**Document Version:** 1.0 (Phase 1: Requirements Analysis)  
**Status:** Approved for Implementation  
**Build System:** Buildroot 2025.02.10 LTS  
**Primary Workload:** CPU-based MobileNetV2 INT8 Quantized Inference  

---

## 1. Executive Summary & Problem Formulation

Edge machine learning devices demand predictable latency, minimal boot times, high energy efficiency, and immunity to filesystem corruption caused by ungraceful power disconnections. General-purpose embedded Linux distributions (such as Raspberry Pi OS, Debian, or Ubuntu Core) are designed for broad peripheral compatibility and developer convenience rather than dedicated real-time inference workloads.

This project designs and implements an **application-specific, stripped-down Linux distribution** engineered using Buildroot for the Raspberry Pi 4 Model B (4GB RAM, ARM64). By stripping out all unnecessary kernel subsystems, eliminating general-purpose userspace daemons (`systemd`, `dbus`, `udev`, package managers), utilizing a hardened read-only root filesystem with OverlayFS, and deploying a native C++ TensorFlow Lite / LiteRT inference pipeline accelerated by ARM NEON and XNNPACK, the system achieves maximum determinism, minimal memory footprint, fast boot times, and resilient edge operation.

---

## 2. Why Stock Linux Is Unsuitable for Dedicated Edge ML Inference

Standard stock distributions (such as Raspberry Pi OS Lite 64-bit or Ubuntu Server) are fundamentally ill-suited for dedicated edge ML appliances due to several structural limitations:

### 2.1 Excessive Boot Latency
- **Service Dependency Chains:** Stock systems rely on `systemd`, which executes extensive service dependency graphs, udev hardware discovery, systemd-journald initialization, network-online waits, cron, and user-session services. Cold boot to userland typically consumes 15 to 30 seconds.
- **Delayed Inference Execution:** When ML applications depend on full multi-user target initialization and multi-gigabyte Python runtime loading, time-to-first-inference frequently exceeds 35 to 50 seconds after power is applied.

### 2.2 Massive Resource Footprint and Memory Bloat
- **RAM Overhead:** Raspberry Pi OS Lite requires 150 MB to 280 MB of RAM simply idling with background services (`systemd-resolved`, `dbus-daemon`, `cron`, `polkitd`, `wpa_supplicant`, `rsyslog`). This robs physical LPDDR4 memory from the ML engine's working set, cache tables, and tensor buffers.
- **Storage Bloat:** Stock distributions occupy 1.8 GB to 4.5 GB of flash storage. The vast majority of installed files (package manager databases, manual pages, multi-language locales, legacy fonts, compilers, Python standard libraries) are completely superfluous for a dedicated inference appliance.

### 2.3 Filesystem Corruption Risk from Abrupt Power Off
- **Read-Write Root Filesystem:** Stock Linux mounts the root filesystem (`/`) read-write (`rw`). Constant journaling, logging to `/var/log`, swap file manipulation, and metadata updates create severe vulnerability to flash corruption if the power supply is severed abruptly during field operation.
- **Recovery Overhead:** A corrupted ext4 dirty-bit forces slow `fsck` recovery upon subsequent boot, or drops the system into an unbootable emergency maintenance shell.

### 2.4 Non-Deterministic CPU Scheduling and Jitter
- **Background Interference:** Periodic tasks, log flushing, cron jobs, network service probes, and background housekeeping cause CPU context-switching spikes, cache thrashing, and memory bus contention that directly degrade inference latency predictability.
- **Thermal Inefficiency:** Continuous background daemon activity keeps CPU cores from entering deeper C-states, elevating baseline power consumption and idle operating temperatures.

### 2.5 Expanded Security Attack Surface
- **Unnecessary Services:** Open ports (e.g. SSH server enabled by default or easily exposed), local privilege escalation vectors through setuid binaries (`sudo`, `pkexec`), and standard default users create substantial security vulnerabilities in unattended edge deployments.

---

## 3. Hardware Requirements

| Subsystem | Specification | Justification & Edge Role |
| :--- | :--- | :--- |
| **Compute Board** | Raspberry Pi 4 Model B (Revision 1.4 or 1.5) | Pinned target hardware for Track A. |
| **SoC** | Broadcom BCM2711 | 28nm Quad-core SoC with dedicated peripherals. |
| **CPU Architecture** | Quad-core ARM Cortex-A72 (ARMv8-A 64-bit) @ 1.5 GHz / 1.8 GHz | High-performance 64-bit compute cores supporting ARM NEON Advanced SIMD. |
| **Vector Unit** | ARM NEON SIMD with FP32 and INT8 dot-product/arithmetic support | Primary engine for CPU-accelerated quantized matrix multiplication. |
| **System Memory** | 4 GB LPDDR4-3200 SDRAM (shared with VideoCore) | Dedicated memory partitioned to reserve maximum bandwidth for CPU ML execution. |
| **GPU/Firmware** | Broadcom VideoCore VI @ 500 MHz | Handles initial stage-2 boot and clock distribution; GPU memory restricted to minimum (64 MB). |
| **Storage Medium** | MicroSD Card (UHS-I / Class 10, >= 16 GB) | Holds boot firmware, kernel, read-only rootfs, and optional writeable data partition. |
| **Debug & Telemetry I/O** | Hardware UART (`ttyAMA0` / `serial0`) @ 115200 8N1 | High-precision serial console for early boot timestamps, logging, and headless evaluation. |
| **Power Budget** | 5V DC via USB-C (Official 15.3W PSU, max 3.0A) | Must maintain stability under continuous 4-core SIMD inference load without undervoltage. |
| **Thermal Budget** | Passive heatsink or 5V fan | Must sustain continuous 4-thread inference without exceeding the 80°C thermal throttling ceiling. |

---

## 4. Performance, Latency, Power, and Footprint Requirements

> [!NOTE]  
> All values listed below represent **Proposed Engineering Targets** for the custom Track A OS design. They will be validated against empirical physical hardware measurements in the benchmarking phase.

### 4.1 Target Performance & Latency Metrics

| Metric | Proposed Target (Track A Custom OS) | Typical Baseline (RPi OS Lite 64-bit) | Evaluation Protocol |
| :--- | :--- | :--- | :--- |
| **Cold Boot Time (Power-on to Init)** | **< 3.0 seconds** | ~15.0 - 22.0 seconds | Hardware serial timestamp from reset to PID 1 |
| **Cold Boot to 1st Inference Ready** | **< 4.5 seconds** | ~35.0 - 50.0 seconds | Wall-clock time to classification completion |
| **Single-Thread Inference Latency ($p_{50}$)** | **~45.0 - 55.0 ms** | ~55.0 - 75.0 ms (Python overhead) | MobileNetV2 INT8 (224x224), 100 runs |
| **Multi-Thread (4T) Inference Latency ($p_{50}$)** | **~15.0 - 22.0 ms** | ~22.0 - 35.0 ms | MobileNetV2 INT8 (224x224), 100 runs |
| **Throughput (4-Thread Steady-State)** | **> 45 FPS** | ~28 - 38 FPS | Continuous frame ingestion pipeline |
| **Inference Latency Jitter (p99 - p50)** | **< 3.5 ms** | > 15.0 ms | Strict determinism under idle background |

### 4.2 Target Resource & Storage Footprint

| Parameter | Proposed Target (Custom OS) | Typical Baseline (RPi OS Lite) | Justification |
| :--- | :--- | :--- | :--- |
| **Total SD Card Image (`sdcard.img`)** | **< 200 MB** | ~1.8 GB - 2.5 GB | Stripped of desktop, apt, python, and doc trees. |
| **RootFS Partition Footprint (Compressed)** | **< 120 MB** | ~1.5 GB | Minimal glibc userspace + TFLite runtime + model. |
| **System Idle RAM Usage** | **< 35 MB** | ~160 MB - 220 MB | No systemd, no dbus, no network daemons. |
| **Active ML Inference Peak RSS** | **< 28 MB** | ~120 MB - 250 MB | Native C++ executable vs Python runtime bloat. |

### 4.3 Target Power & Thermal Characteristics

| Parameter | Proposed Target (Custom OS) | Conditions |
| :--- | :--- | :--- |
| **Idle System Power Draw** | **< 2.4 Watts** | Headless boot, serial console active, cores parked. |
| **Active Inference Power Draw** | **< 5.2 Watts** | 4-thread continuous MobileNetV2 INT8 execution. |
| **Sustained Thermal Steady-State** | **< 68°C** (with passive cooler) | Continuous 10-minute inference loop; no throttling (`0x0`). |

---

## 5. Build System Requirements

1. **Pinned Buildroot Release:**
   - Build system must use stable **Buildroot 2025.02.10 LTS** (Archive SHA256: `f3de928ed368ef3dcadb97d4f4c1a9f49424a367759dba1d97c4c2bc5a22eab1`).
   - All source archives and package tarballs must have cryptographically pinned hash files (`.hash`).
2. **Reproducibility & Out-of-Tree Workflow:**
   - Pinned build timestamps (`BR2_REPRODUCIBLE=y`).
   - Clean separation of source repository, configurations (`configs/`), board overlays (`rootfs/`), and build artifacts (`output/`).
3. **Cross-Compilation Toolchain:**
   - Architecture: ARM64 (`BR2_aarch64=y`, `BR2_cortex_a72=y`).
   - C Library: GNU C Library (`BR2_TOOLCHAIN_USES_GLIBC=y`) to satisfy TensorFlow Lite native runtime requirements.
   - Compiler: GCC >= 12.x (`BR2_TOOLCHAIN_GCC_AT_LEAST_12=y`) with C++17 support and POSIX threads.
   - Shared library support (`BR2_SHARED_LIBS=y`).

---

## 6. Kernel and Driver Requirements

### 6.1 Kernel Tree & Base Configuration
- **Kernel Version:** Linux Kernel 6.6.y (Raspberry Pi LTS branch `rpi-6.6.y`).
- **Device Tree:** Broadcom BCM2711 DTB (`bcm2711-rpi-4-b.dtb`) with support for board revisions 1.4 and 1.5.
- **Base Defconfig:** Minimalist derivate of `bcm2711_defconfig`.

### 6.2 Essential Kernel Configurations
- **Filesystems:**
  - `CONFIG_OVERLAY_FS=y`: Required for volatile writable overlay over the read-only root.
  - `CONFIG_SQUASHFS=y` and `CONFIG_SQUASHFS_XZ=y`: Required for compressed immutable rootfs.
  - `CONFIG_EXT4_FS=y`: Required for writable persistent logging/data partition.
- **Memory & Core Features:**
  - `CONFIG_ARM64_4K_PAGES=y`: 4KB MMU page size for low memory overhead and broad compatibility.
  - `CONFIG_CGROUPS=y` and `CONFIG_NAMESPACES=y`: Process containment and resource accounting.
  - `CONFIG_PRINTK_TIME=y`: High-resolution microsecond boot timestamping in dmesg.
  - `CONFIG_CPU_FREQ_GOV_PERFORMANCE=y`: Default CPU governor locked to maximum frequency to prevent dynamic frequency scaling latency jitter during inference bursts.

### 6.3 Stripped Drivers & Subsystems
To reduce kernel image size, conserve boot time, and minimize attack surface, the following components must be disabled:
- Legacy sound architecture (`CONFIG_SOUND=n`, `CONFIG_SND=n`).
- Unused wireless & Bluetooth stacks in headless inference mode (`CONFIG_BT=n`).
- Unnecessary multimedia / analog TV tuners / legacy framebuffers.
- Unused network protocol families (Appletalk, IPX, Amateur Radio).

---

## 7. ML Runtime and Application Requirements

### 7.1 Framework & Acceleration
- **Framework:** TensorFlow Lite C++ runtime (or LiteRT C++ API).
- **Engine Acceleration:**
  - `XNNPACK` delegate enabled by default for highly optimized ARM NEON SIMD kernels.
  - Multi-threaded CPU execution with runtime-selectable thread pool (1, 2, or 4 threads).
  - Explicit SIMD vectorization targeting ARMv8 Cortex-A72 pipeline.
- **Strictly No Python in Production:**
  - Zero Python interpreters (`python3`), wheels, or packaging tools in the target rootfs.
  - Eliminates ~100MB of userspace bloat and removes 2-5 seconds of interpreter startup overhead.

### 7.2 Model Architecture & Quantification
- **Neural Network:** MobileNetV2 (1.0x alpha width multiplier, 224x224x3 input tensor).
- **Quantization:** Fully quantized INT8 weights and activations (TFLite FlatBuffer format, size ~3.4 MB).
- **Output:** Softmax probabilities over 1001 ImageNet classification labels.

### 7.3 Native C++ Application (`edge-classify`)
- **Language Standard:** Modern C++17.
- **Capabilities:**
  - Single-shot classification mode with microsecond benchmark output.
  - Continuous batch inference mode for sustained thermal and throughput stress testing.
  - Direct raw bitmap / preprocessed tensor ingestion without external bulky image libraries.
  - High-precision telemetry logging using `clock_gettime(CLOCK_MONOTONIC)`.

---

## 8. Security & System Hardening Requirements

### 8.1 Read-Only Root Filesystem Enclosure
- Root filesystem mounted strictly read-only (`ro` kernel command-line and `/etc/fstab`).
- Direct file modifications to system binaries (`/bin`, `/sbin`, `/usr`, `/lib`) are rejected with `EROFS` (Read-only file system).
- Guarantees 100% immunity to bricking or corruption caused by abrupt power loss.

### 8.2 OverlayFS for Volatile State
- Volatile directories (`/tmp`, `/run`, `/var/run`) mounted on RAM-backed `tmpfs`.
- Mutable configuration states (`/etc`) handled via OverlayFS with upper layer in volatile RAM or isolated `/data` partition.

### 8.3 Unprivileged Process Execution
- The ML inference application MUST execute under an unprivileged system user (`edgeai`, UID: 1001, GID: 1001).
- Shell set to `/bin/false` or `/bin/nologin`.
- Process launched with dropped Linux capabilities (`CAP_SYS_ADMIN` and superuser rights completely stripped).

### 8.4 Network Surface Hardening
- Production image disables SSH, Telnet, FTP, and remote shell listeners.
- No listening network ports exposed in default operating mode.
- System maintenance performed via local hardware serial UART (`ttyAMA0`).

---

## 9. Deployment and Benchmarking Requirements

### 9.1 Storage Partitioning & Image Packaging
The build system must generate a unified, flashable disk image (`sdcard.img`) via `genimage` containing:
1. **Partition 1 (`boot`, FAT32, ~64 MB):** VideoCore firmware (`start4.elf`, `fixup4.dat`), `config.txt`, `cmdline.txt`, kernel `Image`, and DTB overlays.
2. **Partition 2 (`rootfs`, ext4 / SquashFS, Read-Only, ~120-180 MB):** Immutable operating system, shared libraries, ML engine, and model assets.
3. **Partition 3 (`data`, ext4, Writeable, ~100 MB or remainder):** Telemetry logs, benchmark outputs, and persistent device parameters.

### 9.2 Comparative Evaluation Protocol
The custom distribution must be empirically evaluated against **Raspberry Pi OS Lite (64-bit Debian Bookworm)** using the experimental design codified in [benchmark_plan.md](benchmark_plan.md):
- Hardware consistency: Identical board, power supply, and SD card media.
- Measured metrics: Boot latency breakdown, idle RAM, active inference RAM, rootfs footprint, inference latency percentiles ($p_{50}, p_{90}, p_{95}, p_{99}$), throughput (FPS), thermal progression, and power-cut resilience.
- Data integrity: All measured values must be accompanied by raw timestamped execution logs; no fabricated numbers.
