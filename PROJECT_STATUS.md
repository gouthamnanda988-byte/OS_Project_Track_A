# Project Status: Edge ML Linux OS (Track A)

**Target Hardware:** Raspberry Pi 4 Model B (4GB RAM)  
**Emulation Target:** QEMU ARM64 Virt Machine (`aarch64`)  
**Build System:** Buildroot 2025.02.10 LTS  
**Primary Workload:** CPU-based MobileNetV2 INT8 Quantized Inference  
**Last Updated:** November 2026  

---

## 1. Current Phase
**Phase 5: Machine Learning Runtime Integration (TensorFlow Lite C++)**  
**Status:** COMPLETED & VERIFIED — Ready for Phase 6 (Edge Inference Application & Camera Pipeline).

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

### Phase 3: Build Initial ARM64 Linux System for QEMU (Approved)
1. Diagnosed and resolved Buildroot error rejecting spaces in `$PATH` caused by WSL importing Windows paths.
2. Removed `host-qemu` from defconfig to leverage host QEMU 8.2.2.
3. Cross-compiled kernel 6.12.27, glibc 2.40, BusyBox 1.37.0, and rootfs image.
4. Verified clean boot to `buildroot login:` in QEMU virt machine.
5. Committed changes to Git (`831bd51`).

### Phase 4: Root Filesystem Hardening & Storage/OverlayFS Setup (Approved)
1. Configured root filesystem to mount read-only (`ro`) in `rootfs/etc/fstab` and `inittab`.
2. Mounted volatile temporary filesystems as `tmpfs` on `/tmp` and `/run`.
3. Created dynamic OverlayFS init script `rootfs/etc/init.d/S00overlay` for `/etc`.
4. Created unprivileged least-privilege user `edgeai` (UID 1001, GID 1001, password locked).
5. Rebuilt rootfs and passed all 7 QEMU verification tests.
6. Committed changes to Git (`f10e4c5`).

### Phase 5: Machine Learning Runtime Integration (Completed & Verified)
1. **Model Acquisition & Verification:**
   - Candidate Model: MobileNetV2 INT8 Quantized (`mobilenet_v2_1.0_224_quant.tflite`).
   - Source: TensorFlow Hosted Models (`https://storage.googleapis.com/download.tensorflow.org/models/tflite_11_05_08/mobilenet_v2_1.0_224_quant.tgz`).
   - Locally Calculated Model SHA256: `f08d447cde49b4e0446428aa921aff0a14ea589fa9c5817b31f83128e9a43c1d`.
   - Labels File: `ml_model/imagenet_labels.txt` (1,001 classes, SHA256: `b8aaeb5630c62626a7d124a05c0e14230a7ffdd2136a31d0190a4fb71c1d82f1`).
2. **Buildroot TensorFlow Lite Integration:**
   - Enabled `BR2_PACKAGE_TENSORFLOW_LITE=y` in `configs/qemu_aarch64_virt_edgeml_defconfig`.
   - Buildroot 2025.02.10 automatically resolved dependencies: `cpuinfo`, `eigen 3.4.0`, `farmhash`, `fft2d`, `flatbuffers 24.12.23`, `gemmlowp`, `libabseil-cpp 20250127.0`, `neon-2-sse`, and `ruy` (Google optimized ARM NEON matrix multiplication engine).
   - Upstream TFLite Version: **2.11.0** (SHA256: `99c732b92b1b37fc243a559e02f9aef5671771e272758aa4aec7f34dc92dac48`).
   - Raspberry Pi 4 configuration was kept strictly untouched.
3. **C++ Inference Test Application:**
   - Developed native C++ inference verification program `tests/tflite_test.cpp` and `tests/Makefile`.
   - Cross-compiled using `aarch64-buildroot-linux-gnu-g++` against sysroot `output_qemu/staging/`.
   - Verified dynamic linker dependencies via host `readelf -d`: dynamically linked to `libtensorflow-lite.so`, `libstdc++.so.6`, `libc.so.6`, `libm.so.6`, and `ld-linux-aarch64.so.1`.
4. **Staging & RootFS Overlay:**
   - Staged stripped binary into `rootfs/usr/bin/tflite_test` (31 KB).
   - Staged model and labels into `rootfs/usr/share/edgeml/models/`.
   - Reassembled `rootfs.ext4` via `./scripts/build.sh build qemu 4`.
5. **Live QEMU Verification & Benchmarking:**
   - Booted operating system in QEMU ARM64 emulator under the constrained 113 MB usable RAM limit.
   - Executed all 10 Phase 5 verification tests with zero failures.

---

## 3. Files Created or Modified

| File | Status | Description |
| :--- | :--- | :--- |
| `configs/qemu_aarch64_virt_edgeml_defconfig` | Modified | Enabled `BR2_PACKAGE_TENSORFLOW_LITE=y` |
| `ml_model/mobilenet_v2_1.0_224_quant.tflite` | Created | Quantized INT8 MobileNetV2 flatbuffer model (3.5 MB) |
| `ml_model/imagenet_labels.txt` | Created | 1001-class ImageNet synset labels file (22 KB) |
| `ml_model/mobilenet_v2_1.0_224_quant_info.txt` | Created | Model metadata summary |
| `tests/tflite_test.cpp` | Created | Native C++ TFLite inference benchmark and verification application |
| `tests/Makefile` | Created | Reproducible Makefile targeting AArch64 cross-toolchain wrapper |
| `rootfs/usr/bin/tflite_test` | Created | Stripped ARM64 inference verification executable |
| `rootfs/usr/share/edgeml/models/*` | Created | Staged model and label files in rootfs overlay |
| `PROJECT_STATUS.md` | Modified | Updated with Phase 5 verification results and metrics |

---

## 4. Commands Executed

```bash
# 1. Download official MobileNetV2 INT8 model archive
wget -c https://storage.googleapis.com/download.tensorflow.org/models/tflite_11_05_08/mobilenet_v2_1.0_224_quant.tgz -O downloads/mobilenet_v2_1.0_224_quant.tgz
sha256sum downloads/mobilenet_v2_1.0_224_quant.tgz

# 2. Extract model and labels
tar -xzf downloads/mobilenet_v2_1.0_224_quant.tgz -C ml_model/

# 3. Configure Buildroot with TensorFlow Lite
./scripts/build.sh configure qemu

# 4. Cross-compile TensorFlow Lite 2.11.0 runtime and dependencies (-j4)
./scripts/build.sh build qemu 4

# 5. Build C++ inference verification binary
make -C tests clean && make -C tests

# 6. Stage demo binary and model into rootfs overlay
mkdir -p rootfs/usr/bin rootfs/usr/share/edgeml/models
cp tests/tflite_test rootfs/usr/bin/ && ./output_qemu/host/bin/aarch64-buildroot-linux-gnu-strip rootfs/usr/bin/tflite_test
cp ml_model/mobilenet_v2_1.0_224_quant.tflite rootfs/usr/share/edgeml/models/
cp ml_model/imagenet_labels.txt rootfs/usr/share/edgeml/models/
chmod 755 rootfs/usr/bin/tflite_test && chmod 644 rootfs/usr/share/edgeml/models/*

# 7. Re-assemble rootfs with overlay
./scripts/build.sh build qemu 4

# 8. Boot QEMU and execute 10 verification tests
./scripts/build.sh run-qemu
```

---

## 5. Build / Test Results

### 1. Generated Build Artifacts (`output_qemu/images/` & `target/`)
- **Kernel Binary:** `output_qemu/images/Image` (Size: **12 MB**) — Linux 6.12.27 ARM64
- **Root Filesystem:** `output_qemu/images/rootfs.ext4` (Size: **60 MB**, ext4)
- **TFLite Shared Library:** `output_qemu/target/usr/lib/libtensorflow-lite.so` (Size: **3.1 MB**, stripped, ARM aarch64)
- **Inference Application:** `rootfs/usr/bin/tflite_test` (Size: **31 KB**, stripped, ARM aarch64)

### 2. Live QEMU Verification Test Output (10 / 10 PASS)

| Test ID | Test Description | Observed Target Terminal Output | Result |
| :--- | :--- | :--- | :--- |
| **Test 1** | TFLite Shared Library | `-rwxr-xr-x 1 root root 3.1M Nov 8 08:56 /usr/lib/libtensorflow-lite.so` | **PASS** |
| **Test 2** | Dynamic Linking | Dynamic linker resolves all libraries; `/usr/bin/tflite_test` executes and prints usage | **PASS** |
| **Test 3** | Model Accessibility | `-rw-r--r-- 1 root root 3.4M ... mobilenet_v2_1.0_224_quant.tflite`<br>`-rw-r--r-- 1 root root 21.2K ... imagenet_labels.txt` | **PASS** |
| **Test 4** | Model Loading | `[PASS] Model FlatBuffer loaded successfully.`<br>`[PASS] TFLite Interpreter instantiated.` | **PASS** |
| **Test 5** | Tensor Allocation & Metadata | `[PASS] AllocateTensors() succeeded.`<br>Input: `[1, 224, 224, 3]`, type `UINT8`, `scale=0.0078125`, `zero_point=128`<br>Output: `[1, 1001]`, type `UINT8`, `scale=0.0988925`, `zero_point=58` | **PASS** |
| **Test 6** | Inference Execution | Full forward pass executed over synthetic test pattern; top-5 classes decoded with labels | **PASS** |
| **Test 7** | Measured Latency | **Measured Benchmark (5 iterations):**<br>• Min Latency: **469.14 ms**<br>• Max Latency: **481.48 ms**<br>• Avg Latency: **475.51 ms** | **PASS** |
| **Test 8** | Measured Memory Footprint | **Measured Process & System Memory:**<br>• Initial VmRSS: **5,120 KB**<br>• Active VmRSS during inference: **13,468 KB (~13.1 MB)**<br>• Virtual Peak (VmPeak): **16,516 KB (~16.1 MB)**<br>• System MemAvailable: **96,496 KB (~94.2 MB free)** | **PASS** |
| **Test 9** | Reboot & Clean Shutdown | `Tearing down OverlayFS on /etc: OK`<br>`reboot: Power down` | **PASS** |
| **Test 10** | Phase 4 Regression Verification | • Rootfs read-only: `/dev/root on / type ext4 (ro,relatime)` (`touch /test_ro_fail` rejected)<br>• tmpfs on `/tmp` and `/run` active<br>• OverlayFS on `/etc` active and writable<br>• `edgeai` user verified: `uid=1001`, `gid=1001`, login locked | **PASS** |

### 3. Output Predictions (Top 5 Classes Observed in Test 6)

```
--- Output Predictions (Top 5 Classes) ---
  #1 Class  557 | Score:     8.01 | Label: fire screen, fireguard
  #2 Class  905 | Score:     7.61 | Label: window screen
  #3 Class  600 | Score:     6.92 | Label: honeycomb
  #4 Class  633 | Score:     6.72 | Label: loudspeaker, speaker, speaker unit, loudspeaker system, speaker system
  #5 Class  829 | Score:     6.53 | Label: strainer
```

---

## 6. Errors and Their Resolutions

1. **Issue:** When building `tests/tflite_test` initially with `make -C tests`, compilation failed with syntax errors regarding `__Float32x4_t` in `bits/math-vector.h`.  
   **Root Cause:** Host `g++` was invoked instead of the cross-compiler because `CXX ?= ...` in the Makefile did not override the environment's default host `CXX`. Host x86_64 GCC cannot process ARM64 Neon vector headers.  
   **Resolution:** Modified `tests/Makefile` to explicitly assign `CXX = ../output_qemu/host/bin/aarch64-buildroot-linux-gnu-g++`. Compilation succeeded cleanly.

---

## 7. Remaining Tasks

- [x] **Phase 0:** Workspace Inspection, Environment Verification & Architectural Foundation (COMPLETED & VERIFIED)
- [x] **Phase 1:** Requirements Analysis & Architectural Specification (COMPLETED & VERIFIED)
- [x] **Phase 2:** Buildroot Configuration and Target Preparation (COMPLETED & VERIFIED)
- [x] **Phase 3:** Build Initial ARM64 Linux System for QEMU (COMPLETED & VERIFIED)
- [x] **Phase 4:** Root Filesystem Hardening & Storage/OverlayFS Setup (COMPLETED & VERIFIED)
- [x] **Phase 5:** Machine Learning Runtime Integration (COMPLETED & VERIFIED)
- [ ] **Phase 6:** Edge Inference Application (Develop native C++ inference engine `edge-classify`, cross-compile and stage).
- [ ] **Phase 7:** Target A Hardware Build (Build full Raspberry Pi 4 Model B kernel 6.6, BCM2711 DTB, firmware, and generate `sdcard.img`).
- [ ] **Phase 8:** Target Deployment, Verification & Comparative Benchmarking (Empirical benchmarking against Raspberry Pi OS Lite).

---

## 8. Exact Next Step

**Awaiting user approval to proceed to Phase 6:**  
Develop the complete edge image classification application (`edge-classify`) in C++, implementing image ingestion, preprocessing (resizing and normalization), inference pipelining, and automated benchmarking.
