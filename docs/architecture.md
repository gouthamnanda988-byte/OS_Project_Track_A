# System Architecture: Track A — Edge ML Linux OS

**Project Title:** Application-Specific Linux Distribution for ML Inference at the Edge  
**Track:** Track A (Target: Raspberry Pi 4 Model B, 4GB RAM — ARM64)  
**Document Version:** 1.0 (Phase 1: Requirements Analysis)  
**Status:** Approved for Implementation  
**Build System:** Buildroot 2025.02.10 LTS  

---

## 1. End-to-End System Architecture

The custom Edge ML Linux distribution is structured into distinct, decoupled architectural layers designed to ensure maximum determinism, minimal memory consumption, zero flash wear, and unprivileged ML execution.

```mermaid
flowchart TD
    subgraph AppLayer ["Layer 5: Application & Edge ML Inference Layer"]
        InferenceApp["Native C++ Engine: edge-classify"]
        TFLiteRuntime["TensorFlow Lite / LiteRT C++ Runtime"]
        XNNPACKEngine["XNNPACK SIMD Vector Engine"]
        NEONKernels["ARM64 NEON Vectorized INT8 Kernels"]
        ModelFlatBuffer["MobileNetV2 INT8 FlatBuffer (3.4 MB)"]
        UserContext["Security Context: User edgeai (UID 1001, GID 1001)"]
        
        UserContext --> InferenceApp
        InferenceApp --> TFLiteRuntime
        TFLiteRuntime --> XNNPACKEngine
        XNNPACKEngine --> NEONKernels
        ModelFlatBuffer --> TFLiteRuntime
    end

    subgraph UserSpace ["Layer 4: Userspace & Service Orchestration Layer"]
        BusyBoxInit["BusyBox /sbin/init (PID 1)"]
        Inittab["Configuration: /etc/inittab"]
        SysInit["SysV rcS Script: /etc/init.d/rcS"]
        S01Mount["S01mountfs (Mount devtmpfs, proc, sysfs, tmpfs)"]
        S10Overlay["S10overlay (Mount OverlayFS on /etc)"]
        S99Inference["S99edge-inference (Drop privileges, exec edge-classify)"]
        CLibrary["GNU C Library: glibc 2.40+"]
        
        BusyBoxInit --> Inittab
        Inittab --> SysInit
        SysInit --> S01Mount
        SysInit --> S10Overlay
        SysInit --> S99Inference
        S99Inference --> UserContext
        InferenceApp -.-> CLibrary
    end

    subgraph StorageLayer ["Layer 3: Storage & Virtual Filesystem (VFS) Layer"]
        VFS["Linux Virtual Filesystem (VFS)"]
        RO_Root["Partition 2: RootFS (SquashFS / ext4, READ-ONLY)"]
        Volatile_RAM["RAM tmpfs: /tmp, /run, /var"]
        Data_Partition["Partition 3: Data (ext4, Writeable /data)"]
        OverlayFS_Mount["OverlayFS Layer: merged=/etc, lowerdir=/etc (ro), upperdir=/data/etc"]
        
        VFS --> RO_Root
        VFS --> Volatile_RAM
        VFS --> Data_Partition
        VFS --> OverlayFS_Mount
    end

    subgraph KernelLayer ["Layer 2: Linux Kernel Layer (ARM64)"]
        LinuxKernel["Linux Kernel 6.6.y (bcm2711_defconfig stripped)"]
        CoreDrivers["BCM2711 Platform Drivers (UART, Clocks, SDHCI MMC)"]
        FSModules["Filesystem Drivers (SquashFS, OverlayFS, ext4)"]
        PerfGovernor["CPUFreq Governor: Locked Performance Mode"]
        DeviceTree["Device Tree: bcm2711-rpi-4-b.dtb"]
        
        LinuxKernel --> CoreDrivers
        LinuxKernel --> FSModules
        LinuxKernel --> PerfGovernor
        DeviceTree --> LinuxKernel
    end

    subgraph HardwareLayer ["Layer 1: Hardware & Low-Level Firmware"]
        BootROM["Broadcom BCM2711 On-Chip Boot ROM"]
        SPI_EEPROM["Raspberry Pi 4 SPI EEPROM Bootloader"]
        VideoCoreFW["VideoCore VI Firmware (bootcode.bin, start4.elf)"]
        HW_SoC["Broadcom BCM2711 SoC: 4x Cortex-A72 @ 1.5GHz (ARMv8-A + NEON)"]
        HW_RAM["4 GB LPDDR4-3200 SDRAM"]
        MicroSD["MicroSD Storage Medium (UHS-I Class 10)"]
        
        BootROM --> SPI_EEPROM
        SPI_EEPROM --> VideoCoreFW
        VideoCoreFW --> HW_SoC
        HW_SoC --> HW_RAM
        HW_SoC --> MicroSD
    end

    HardwareLayer --> KernelLayer
    KernelLayer --> StorageLayer
    StorageLayer --> UserSpace
    UserSpace --> AppLayer
```

---

## 2. Boot Flow & State Progression

The boot architecture is engineered to bypass all unnecessary multi-user targets and run-level transitions, launching the ML inference pipeline directly into memory within a target cold-boot window of under 4.5 seconds.

```mermaid
flowchart TD
    A["Power-On / Hardware Reset"] --> B["Stage 1: BCM2711 On-Chip ROM probes SPI EEPROM"]
    B --> C["Stage 2: SPI EEPROM initializes LPDDR4 memory controller and MMC"]
    C --> D["Stage 3: VideoCore VI executes start4.elf firmware from FAT32 boot partition"]
    D --> E["Firmware loads config.txt, cmdline.txt, bcm2711-rpi-4-b.dtb, and kernel Image"]
    E --> F["Firmware releases Cortex-A72 cores from reset at EL2"]
    F --> G["Stage 4: Linux Kernel 6.6.y initializes MMU, GIC-400, timers, and SDHCI"]
    G --> H["Kernel mounts RootFS partition /dev/mmcblk0p2 strictly READ-ONLY (ro)"]
    H --> I["Kernel executes PID 1: /sbin/init (BusyBox)"]
    I --> J["Stage 5: /sbin/init parses /etc/inittab and invokes /etc/init.d/rcS"]
    J --> K["rcS mounts /proc, /sys, /dev, and tmpfs on /tmp and /run"]
    K --> L["rcS initializes OverlayFS for mutable configuration state"]
    L --> M["Stage 6: /etc/init.d/S99edge-inference drops privileges to user edgeai"]
    M --> N["Executes native C++ binary /usr/bin/edge-classify"]
    N --> O["TFLite maps model flatbuffer, binds XNNPACK delegate, allocates tensors"]
    O --> P["Stage 7: Target Cold Boot Complete — 1st Inference Ready (< 4.5s Target)"]
```

---

## 3. Storage Partitioning & OverlayFS Architecture

To prevent filesystem degradation or data corruption during field power failures, the physical storage is segregated into three distinct partitions.

```mermaid
flowchart LR
    subgraph PhysicalSD ["Physical MicroSD Storage Layout"]
        P1["Partition 1: BOOT<br/>FAT32 (~64 MB)<br/>start4.elf, config.txt, cmdline.txt,<br/>Image, bcm2711-rpi-4-b.dtb"]
        P2["Partition 2: ROOTFS<br/>SquashFS / ext4 (ro) (~120-180 MB)<br/>Immutable binaries, glibc, TFLite,<br/>MobileNetV2 flatbuffer model"]
        P3["Partition 3: DATA<br/>ext4 (rw) (~100 MB or remainder)<br/>Persistent telemetry logs,<br/>benchmark json output"]
    end

    subgraph VFS_Mounts ["Target Virtual Filesystem Mounts"]
        M_Boot["/boot (Mounted ro)"]
        M_Root["/ (Root Directory, Mounted ro)"]
        M_Tmp["/tmp & /run (tmpfs in RAM)"]
        M_Data["/data (Mounted rw)"]
        M_Etc["/etc (OverlayFS Virtual Mount)"]
    end

    P1 -.->|Mounted Read-Only| M_Boot
    P2 -.->|Mounted Read-Only| M_Root
    P3 -.->|Mounted Read-Write| M_Data

    subgraph OverlayMechanics ["OverlayFS Mechanism for Mutable State"]
        Lower["lowerdir: /etc on Partition 2 (ro)"]
        Upper["upperdir: /data/overlay/etc_upper (rw)"]
        Work["workdir: /data/overlay/etc_work"]
        Lower --> M_Etc
        Upper --> M_Etc
        Work --> M_Etc
    end
```

### Storage Characteristics
1. **Partition 1 (BOOT):** FAT32 formatted, contains only signed firmware, boot parameters, and the compressed kernel binary.
2. **Partition 2 (ROOTFS):** Read-only immutable block device. Any write attempt (`touch /usr/test`) triggers an immediate kernel VFS error (`EROFS: Read-only file system`). Flash memory cell wear is zero.
3. **Partition 3 (DATA):** Optional writable ext4 partition reserved exclusively for metric logging. An ungraceful power loss on `/data` will never prevent the operating system from booting or the inference engine from functioning.

---

## 4. Machine Learning Inference Pipeline Architecture

The edge classification pipeline is implemented in pure C++17 to avoid the multi-megabyte memory overhead and non-deterministic garbage collection cycles of Python.

```mermaid
flowchart TD
    subgraph InputPipeline ["1. Input Ingestion Stage"]
        InputSource["Input Image Frame (224x224 RGB)"]
        Preprocessor["Minimal C++ Preprocessor / Normalizer"]
        InputSource --> Preprocessor
        Preprocessor --> TensorBuffer["Contiguous Tensor Buffer (1 x 224 x 224 x 3 INT8)"]
    end

    subgraph InferenceEngine ["2. TFLite & XNNPACK Acceleration Stage"]
        ModelStorage["/usr/share/models/mobilenet_v2_1.0_224_quant.tflite"]
        ModelStorage --> FlatBufferModel["FlatBuffer Model Loader (Zero-Copy)"]
        FlatBufferModel --> Interpreter["TFLite Interpreter (C++ API)"]
        TensorBuffer --> Interpreter
        
        Interpreter --> DelegateMgr["Delegate Manager"]
        DelegateMgr --> XNNPACK["XNNPACK Execution Delegate"]
        XNNPACK --> ThreadPool["4-Core Thread Pool (Cortex-A72)"]
        ThreadPool --> SIMD_NEON["ARM64 NEON Vectorized Kernel Operations"]
    end

    subgraph OutputPipeline ["3. Classification & Telemetry Stage"]
        SIMD_NEON --> RawOutput["Output Tensor (1 x 1001 INT8 Probabilities)"]
        RawOutput --> PostProcessor["Top-K Softmax Dequantizer & ArgMax"]
        PostProcessor --> ClassResult["Predicted Class ID & Confidence Score"]
        
        Timer["High-Res Clock: clock_gettime(CLOCK_MONOTONIC)"]
        Timer -.-> LatencyCalc["Latency Logger (p50, p95, p99 ms)"]
        PostProcessor --> LatencyCalc
        LatencyCalc --> LogOutput["/data/logs/benchmark_results.json"]
    end
```

---

## 5. Security & Privilege Separation Architecture

The operating system enforces the principle of least privilege across all user and system boundaries.

```mermaid
flowchart TD
    subgraph BootPrivilege ["Privilege Dropping Sequence"]
        KernelExec["Kernel spawns /sbin/init (UID 0: Root)"]
        rcSExec["rcS mounts filesystems as root"]
        InitScript["/etc/init.d/S99edge-inference"]
        
        KernelExec --> rcSExec
        rcSExec --> InitScript
    end

    subgraph PrivilegeDrop ["Drop Superuser Rights"]
        InitScript --> DropExec["exec su -s /bin/sh edgeai -c edge-classify"]
    end

    subgraph UnprivilegedBoundary ["Unprivileged Security Domain: User 'edgeai'"]
        Process["Process: edge-classify (UID 1001, GID 1001)"]
        DropExec --> Process
        
        PermCheck1["Write attempt to /usr, /bin, /sbin -> REJECTED (EROFS & DAC)"]
        PermCheck2["Raw network socket creation -> REJECTED (CAP_NET_RAW absent)"]
        PermCheck3["Kernel module loading -> REJECTED (CAP_SYS_MODULE absent)"]
        PermAllowed["Write telemetry to /data/logs/ -> ALLOWED (Group 1001 Write)"]
        
        Process -.-> PermCheck1
        Process -.-> PermCheck2
        Process -.-> PermCheck3
        Process --> PermAllowed
    end
```

### Security Boundary Matrix

| Resource | Root (`UID 0`, Init Phase) | `edgeai` (`UID 1001`, Runtime Phase) | Protection Mechanism |
| :--- | :--- | :--- | :--- |
| **RootFS Partition (`/`)** | Read-Only | Read-Only (Forbidden by VFS) | VFS `MS_RDONLY` Mount Flag |
| **Application Binary (`/usr/bin`)** | Read-Execute (`0755`) | Read-Execute (`0555`) | Standard POSIX DAC |
| **Model Weight File** | Read-Only (`0644`) | Read-Only (`0444`) | Standard POSIX DAC |
| **Log Directory (`/data/logs`)** | Read-Write | Read-Write (`0775`) | Group Ownership `edgeai` |
| **Hardware Serial Console** | Direct Console Access | No Access | Restricted to root on `ttyAMA0` |
| **Network Interfaces** | Down / No Listener | No Access / No Sockets | No daemons listening |
| **Kernel Capabilities** | All Capabilities | `0` (Zero Capabilities) | Cleared during `su` privilege drop |

---

## 6. Buildroot Out-of-Tree Integration Design

To guarantee 100% build reproducibility and isolate generated binaries from tracked repository sources:

```
OS_Project_Track_A/
├── buildroot/
│   ├── buildroot-2025.02.10.tar.gz   # Pinned release source
│   └── buildroot-2025.02.10/          # Extracted Buildroot engine (gitignored)
├── configs/
│   └── raspberrypi4_64_edgeml_defconfig  # Pinned Buildroot configuration
├── rootfs/                            # Root filesystem overlay tree
│   ├── etc/
│   │   ├── inittab                    # BusyBox init configuration
│   │   ├── fstab                      # Mount declarations (ro rootfs, tmpfs)
│   │   └── init.d/
│   │       ├── rcS                    # Sysinit orchestrator
│   │       ├── S01mountfs             # Early VFS mounter
│   │       ├── S10overlay             # OverlayFS initialization
│   │       └── S99edge-inference      # Service startup & privilege dropper
│   └── usr/
│       └── share/models/              # MobileNetV2 INT8 model and label maps
├── scripts/
│   ├── check_host_env.sh              # Host audit tool
│   ├── build.sh                       # Wrapper: make O=output ...
│   └── run_benchmarks.sh              # Empirical measurement harness
└── output/                            # Out-of-tree build directory (gitignored)
    ├── build/                         # Package build directories
    ├── host/                          # Cross-toolchain (aarch64-buildroot-linux-gnu)
    ├── target/                        # Staged root filesystem
    └── images/                        # Final artifacts: Image, rootfs.ext4, sdcard.img
```

All build commands are executed out-of-tree using:
```bash
make -C buildroot/buildroot-2025.02.10 \
    O=$(pwd)/output \
    BR2_DEFCONFIG=$(pwd)/configs/raspberrypi4_64_edgeml_defconfig \
    defconfig
```
This guarantees clean separation, rapid cleaning (`rm -rf output/`), and immutable tracked configuration files.
