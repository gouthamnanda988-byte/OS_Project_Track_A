# Phase 7: Edge ML System Integration & Evaluation

## A. Project Overview

### 1. Problem Statement
Deploying deep learning computer vision models on resource-constrained embedded edge hardware (such as the Raspberry Pi 4 Model B) poses significant systems engineering challenges:
- **General-Purpose OS Bloat:** Standard desktop/server distributions (e.g., standard Raspberry Pi OS or Ubuntu Server) consume 250 MB–500 MB of RAM at idle, ship with hundreds of unneeded background services (`systemd`, Avahi, Bluetooth, CUPS), and exhibit slow cold-boot times (20–40 seconds).
- **Storage Degradation:** Standard distributions continuously write logs and state to flash media (SD cards), causing filesystem corruption during sudden power failures in field deployments.
- **Security Vulnerabilities:** Monolithic embedded designs frequently run edge inference workloads with elevated (root) privileges, expanding the attack surface if network interfaces are exposed.

### 2. Project Objectives (Track A)
Build a purpose-built, highly secure, minimal embedded Linux operating system for edge machine learning inference:
- Construct a reproducible minimal system using **Buildroot 2025.02.10 LTS**, **Linux Kernel 6.12.27 ARM64**, and **BusyBox 1.37.0**.
- Enforce immutability and durability via a **Read-Only Root Filesystem** paired with a volatile **tmpfs** layer and **OverlayFS on `/etc`** to absorb runtime state changes without wearing out flash storage.
- Isolate execution to a dedicated unprivileged user (`edgeai`, UID/GID 1001) following the principle of least privilege.
- Integrate the **TensorFlow Lite 2.11.0 C++ Runtime** accelerated with Google's ARM NEON `ruy` matrix multiplication engine.
- Deploy an INT8-quantized **MobileNetV2** image classification model and build a production C++ inference engine (`edge-classify`) with automated benchmarking and usability tooling.

### 3. System Architecture & Environment
- **Host System:** Ubuntu 24.04.4 LTS on WSL2 (x86_64, 20 vCPUs, 7.6 GB RAM)
- **Target Hardware Architecture:** ARM64 / AArch64 (Raspberry Pi 4 Model B profile, Cortex-A53/A72)
- **Target Emulation Platform:** QEMU ARM64 `virt` machine (`-cpu cortex-a53 -smp 2`, 128 MB RAM limit)
- **Kernel:** Linux 6.12.27 ARM64 (SMP enabled, 12 MB Image)
- **Root Filesystem:** ext4 (60 MB raw partition image)
- **Compiler Toolchain:** GCC 13.4.0, glibc 2.40, C++17 support

---

## B. Phase 7 Implementation

### 1. Boot-Time Service Integration
To evaluate safe and practical boot integration:
- Designed `/etc/init.d/S99edgeml` under the BusyBox SysV-style init sequence (`/etc/init.d/rcS`).
- Service configuration is externalized in `/etc/default/edgeml`.
- **Fail-Safe Boot Guard:** By default, `EDGEML_ENABLED="no"`. The system boots to the login prompt in under 2 seconds without executing unsolicited inferences.
- **Non-Blocking Background Spawning:** When enabled (`EDGEML_ENABLED="yes"`), the service verifies model and image paths, drops privileges to `edgeai`, and forks the inference task in the background (`&`) logging to `/tmp/edgeml-service.log`. This guarantees that image processing delays or errors never stall the system boot sequence or prevent terminal login.
- **Management Interface:** Supports standard SysV verbs:
  - `start`: Starts service or reports disabled status.
  - `stop`: Gracefully stops the service.
  - `status`: Displays current parameters (model, default image, user, thread count) and recent log outputs.
  - `run`: Triggers an immediate, interactive on-demand inference pass as `edgeai`.

### 2. Edge ML Application Usability
Developed `/usr/bin/edgeml-run` to streamline developer and operator workflows:
- Automatically resolves default model (`mobilenet_v2_1.0_224_quant.tflite`) and labels (`imagenet_labels.txt`).
- Supports shorthand image resolution from the system asset repository (`/usr/share/edgeml/images/`).
- Built-in helper flags:
  - `-l, --list-images`: Discovers and enumerates installed test images.
  - `-t, --threads <N>`: Selects inference thread pool size.
  - `-k, --top <K>`: Configures prediction output depth.
  - `-b, --benchmark [N]`: Runs multi-iteration performance benchmarking.
- Operates under both `root` and unprivileged `edgeai` user accounts.

### 3. Reliability & Defensive Validation
- Implemented defensive argument validation: explicit rejection of negative or zero thread/top-$K$ counts (`--threads 0`), non-existent models, and unreadable images.
- Distinct application exit codes:
  - `0`: Successful execution.
  - `1`: Invalid CLI arguments or syntax.
  - `2`: Model loading failure.
  - `6`: Image loading or format parsing failure.
- Conducted 10 consecutive standalone inference passes to evaluate process lifecycle stability and verify absence of memory leaks.

---

## C. Architecture & Inference Data Flow

```
+-------------------------------------------------------------------------+
|                              Image Input                                |
|  PPM Image File (Binary P6 or ASCII P3 from /usr/share/edgeml/images/)  |
+-------------------------------------------------------------------------+
                                    │
                                    ▼
+-------------------------------------------------------------------------+
|                      ImageLoader (image_loader.cpp)                     |
|  • Sanitizes header comments (#) and tokenizes dimensions (W, H, MaxVal)|
|  • Ingests 24-bit raw RGB pixel payloads with strict byte bounds check  |
+-------------------------------------------------------------------------+
                                    │
                                    ▼
+-------------------------------------------------------------------------+
|                  ImagePreprocessor (image_preprocessor.cpp)             |
|  • Resizes arbitrary input resolutions to 224x224 via bilinear interp   |
|  • Queries tensor scale (0.0078125) and zero_point (128)                |
|  • Quantizes [0..255] RGB bytes: q = round((norm / scale)) + zero_point |
+-------------------------------------------------------------------------+
                                    │
                                    ▼
+-------------------------------------------------------------------------+
|                     TensorFlow Lite 2.11.0 Runtime                      |
|  • Configured thread pool (N=1 or N=2 POSIX worker threads)             |
|  • Evaluates 3.5 MB MobileNetV2 INT8 FlatBuffer model                   |
|  • Executes GEMM operations via ARM NEON 'ruy' matrix math engine       |
+-------------------------------------------------------------------------+
                                    │
                                    ▼
+-------------------------------------------------------------------------+
|                     Top-K Prediction & Dequantization                   |
|  • Dequantizes output tensor logits: score = (q - zp) * scale           |
|  • Sorts descending confidence scores and maps to ImageNet synsets      |
+-------------------------------------------------------------------------+
                                    │
                                    ▼
+-------------------------------------------------------------------------+
|                             Terminal Output                             |
|  • Prints Top-K Rank / Class ID / Confidence / Label Table              |
|  • Reports Monotonic Latency (Min, Max, Avg, Median) & Throughput (FPS) |
|  • Reports Memory Footprint: VmRSS, VmPeak (/proc), MemAvailable        |
+-------------------------------------------------------------------------+
```

---

## D. Empirical Evaluation Results

All evaluation data was collected directly on the running target operating system within QEMU ARM64 (`-smp 2`, Cortex-A53, 128 MB RAM).

### 1. Performance Benchmark Results

| Metric | Single-Thread (`-t 1`) | Multi-Thread (`-t 2`) | Measurement Method |
| :--- | :--- | :--- | :--- |
| **Average Inference Latency** | **475.41 ms** | **256.89 ms** | 10-iteration benchmark monotonic clock |
| **Median Latency** | 475.62 ms | 257.48 ms | 10-iteration benchmark monotonic clock |
| **Minimum Latency** | 471.18 ms | 252.79 ms | 10-iteration benchmark monotonic clock |
| **Maximum Latency** | 481.56 ms | 259.21 ms | 10-iteration benchmark monotonic clock |
| **Inference Throughput** | **2.10 inf/sec (FPS)** | **3.89 inf/sec (FPS)** | 10-iteration completed throughput |
| **Multi-Core Speedup** | Baseline ($1.00\times$) | **$1.85\times$ Speedup** | $475.41\text{ ms} / 256.89\text{ ms}$ ($-46.0\%$ latency) |
| **Process Active VmRSS** | 13,624 KB (~13.3 MB) | 13,576 KB (~13.3 MB) | `/proc/self/status` |
| **Process Peak Virtual (VmPeak)** | 16,724 KB (~16.3 MB) | 155,556 KB | Virtual thread stack allocations |
| **Image Ingestion Latency** | 5.40 ms | 5.43 ms | High-resolution timer (`std::chrono`) |
| **Resize & Quantization Latency**| 15.50 ms | 14.46 ms | Bilinear scaling ($300\times 200 \to 224\times 224$) |
| **Total Preprocessing Latency** | **20.90 ms** | **19.89 ms** | Ingestion + Resize + Quantization |
| **Startup to First Inference** | ~495 ms | ~277 ms | Model load (12 ms) + Alloc (1.5 ms) + Warmup |
| **System MemAvailable** | 90,984 KB (~88.8 MB free)| 93,524 KB (~91.3 MB free)| `/proc/meminfo` |

### 2. Reliability & Lifecycle Evaluation (10 Consecutive Passes)

To verify that the application releases all allocated resources cleanly without leaking kernel memory or file descriptors, 10 separate process invocations were executed sequentially:

| Run Number | Measured Average Latency | Process Active Resident Memory (VmRSS) |
| :---: | :---: | :---: |
| Run 1 | 475.25 ms | 13,624 KB |
| Run 2 | 478.98 ms | 13,692 KB |
| Run 3 | 475.10 ms | 13,624 KB |
| Run 4 | 475.62 ms | 13,616 KB |
| Run 5 | 470.88 ms | 13,616 KB |
| Run 6 | 475.96 ms | 13,624 KB |
| Run 7 | 486.36 ms | 13,596 KB |
| Run 8 | 470.34 ms | 13,624 KB |
| Run 9 | 469.48 ms | 13,624 KB |
| Run 10 | 468.26 ms | 13,648 KB |

**Observations:**
- Active resident set size (`VmRSS`) remained constant across all 10 executions ($\Delta < 96\text{ KB}$ variation due to dynamic linker page alignment).
- No progressive memory bloat occurred, proving clean memory release on process termination.
- Average inference latency across all 10 independent runs remained stable at $474.62\text{ ms} \pm 5.2\text{ ms}$.

### 3. Consolidated Verification Test Matrix (15 / 15 PASS)

The consolidated test suite [`scripts/phase7_verify.sh`](file:///home/gouth/OS_Project_Track_A/scripts/phase7_verify.sh) executed all integration, usability, reliability, security, and backward compatibility tests live in QEMU:

| Test ID | Test Description | Target Command / Observed Verification | Result |
| :---: | :--- | :--- | :---: |
| **01** | Boot Service Integration | `/etc/init.d/S99edgeml status` confirms disabled by default; non-blocking | **PASS** |
| **02** | Usability Helper Tool | `/usr/bin/edgeml-run -h` prints options; `-l` enumerates installed images | **PASS** |
| **03** | Unprivileged User Execution | `su -s /bin/sh edgeai -c "edgeml-run sample.ppm -k 3"` executes cleanly | **PASS** |
| **04** | Service On-Demand Trigger | `/etc/init.d/S99edgeml run` runs inference as `edgeai` on demand | **PASS** |
| **05** | Error Handling: Missing Model | Rejects `/nonexistent.tflite` with code `2` (`[ERROR] Failed to load model`) | **PASS** |
| **06** | Error Handling: Missing Image | Rejects `/nonexistent.ppm` with code `6` (`[ERROR] Image loading failed`) | **PASS** |
| **07** | Error Handling: Invalid Threads | Rejects `--threads 0` with code `1` (`[ERROR] Invalid thread count: 0`) | **PASS** |
| **08** | Reliability: 10x Inferences | Executed 10 consecutive runs with zero crashes and stable VmRSS | **PASS** |
| **09** | Performance: 1 Thread | 10 iterations: Min 471.18 ms, Max 481.56 ms, Avg 475.41 ms, 2.10 FPS | **PASS** |
| **10** | Performance: 2 Threads | 10 iterations: Min 252.79 ms, Max 259.21 ms, Avg 256.89 ms, 3.89 FPS (1.85x) | **PASS** |
| **11** | Phase 5 Backward Compat | `/usr/bin/tflite_test` runs forward pass with exact matching predictions | **PASS** |
| **12** | Phase 6 Backward Compat | Ingests ASCII P3 PPM with comments (`sample_ascii_64x64.ppm`), outputs Top-3 | **PASS** |
| **13** | Security: Read-Only Rootfs | `touch /test_ro_fail` rejected with `Read-only file system` (`ROOTFS_READONLY_OK`) | **PASS** |
| **14** | Security: OverlayFS on `/etc` | `overlay on /etc` active; `touch /etc/phase7_overlay_test` succeeds | **PASS** |
| **15** | Clean System Teardown | `poweroff` cleanly tears down OverlayFS and powers down kernel | **PASS** |

---

## E. System Limitations

1. **Virtualization vs Physical Silicon:** Benchmarks were recorded under QEMU ARM64 software emulation (`virt` board with Cortex-A53). Emulated CPU cycles do not reflect physical Cortex-A72 hardware memory bandwidth, L1/L2 cache latency, or real-world thermal throttling.
2. **Uncompressed Image Format (PPM):** The image pipeline natively supports Netpbm PPM (P6 binary and P3 ASCII). While this eliminates heavy dependencies like `libpng` or `libjpeg`, PPM files are uncompressed ($150\text{ KB}–180\text{ KB}$ for small images), which would be inefficient for large image datasets.
3. **Model Specificity:** The current pipeline is tailored to MobileNetV2 $224 \times 224 \times 3$ INT8 with 1,001 ImageNet classes. While quantization handling is dynamic, other model architectures (e.g., YOLO object detection) require specialized non-maximum suppression (NMS) decoders.
4. **Synthetic Test Data vs Real Cameras:** Inputs evaluated are synthetic test patterns and color bars. Model classifications (`digital clock`, `lampshade`, `padlock`) reflect pattern activations rather than real-world photographs.

---

## F. Future Scope

1. **Target A Physical Hardware Deployment (Phase 8):**
   - Flash generated `sdcard.img` to physical microSD media.
   - Boot physical Raspberry Pi 4 Model B (4GB RAM) with broadcom GPU firmware and device tree (`bcm2711-rpi-4-b.dtb`).
   - Re-evaluate performance under native hardware ARM NEON SIMD.
2. **Camera Ingestion Pipeline:**
   - Integrate Linux Video4Linux2 (`v4l2`) API or `libcamera` to stream frames directly from the Raspberry Pi Camera Module into the preprocessor buffer.
3. **Compressed Media Ingestion:**
   - Add a lightweight single-file JPEG decoder (e.g. `stb_image.h`) to support JPEG/PNG without bloating system shared libraries.
4. **Hardware Acceleration:**
   - Explore GPU acceleration via Vulkan / OpenCL delegate or external USB accelerators (e.g. Google Coral Edge TPU).
