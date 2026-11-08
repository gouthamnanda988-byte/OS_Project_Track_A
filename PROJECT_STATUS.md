# Project Status: Edge ML Linux OS (Track A)

**Target Hardware:** Raspberry Pi 4 Model B (4GB RAM)  
**Emulation Target:** QEMU ARM64 Virt Machine (`aarch64`)  
**Build System:** Buildroot 2025.02.10 LTS  
**Primary Workload:** CPU-based MobileNetV2 INT8 Quantized Inference  
**Last Updated:** November 2026  

---

## 1. Current Phase
**Phase 6: Edge Inference Application (`edge-classify`)**  
**Status:** COMPLETED & VERIFIED — Ready for Phase 7 (Target A Hardware Build: Raspberry Pi 4 Model B).

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

### Phase 6: Edge Inference Application (`edge-classify`) (Completed & Verified)
1. **Application Architecture & Core Components:**
   - Developed native C++ edge inference application `edge-classify` in `src/`.
   - Implemented `ImageLoader` (`src/image_loader.h`, `src/image_loader.cpp`) supporting Portable Pixmap (PPM) P6 (binary 8/16-bit) and P3 (ASCII) formats with comment stripping and integrity verification.
   - Implemented `ImagePreprocessor` (`src/image_preprocessor.h`, `src/image_preprocessor.cpp`) with bilinear interpolation resizing to arbitrary model geometries ($224 \times 224 \times 3$) and dynamic input tensor quantization supporting `UINT8`, `INT8`, and `FLOAT32`.
   - Implemented full CLI tool `src/edge_classify.cpp` supporting `--image`, `--model`, `--labels`, `--threads`, `--top`, and `--benchmark [N]`.
   - Embedded real-time memory introspection reading `/proc/self/status` (`VmRSS`, `VmPeak`) and `/proc/meminfo` (`MemAvailable`).
2. **Cross-Compilation & Staging:**
   - Authored reproducible `src/Makefile` targeting the Buildroot AArch64 GCC 13.4 cross-compiler wrapper.
   - Stripped executable to a compact **66.1 KB** and staged into rootfs overlay `rootfs/usr/bin/edge-classify`.
   - Developed test image generator `scripts/generate_test_images.py` producing sample patterns (`sample.ppm`, `sample_gradient_300x200.ppm`, `sample_bars_224x224.ppm`, `sample_ascii_64x64.ppm`) staged into `rootfs/usr/share/edgeml/images/`.
   - Reassembled root filesystem image `output_qemu/images/rootfs.ext4` via `./scripts/build.sh build qemu 4`.
3. **Live QEMU Verification & Benchmarking (12 / 12 Tests PASS):**
   - Booted target operating system in QEMU ARM64 (`-smp 2`).
   - Verified CLI help and defensive validation (rejection of non-existent models, images, and invalid thread count).
   - Demonstrated **1.87x multi-thread scaling**: reduced average latency from **471.95 ms** (1 thread) to **252.07 ms** (2 threads), boosting throughput from **2.12 FPS** to **3.97 FPS**.
   - Verified resident active memory remains constrained at **13,624 KB (~13.3 MB)** with >91 MB system RAM available.
   - Verified unprivileged user execution (`su -s /bin/sh edgeai`) without permission errors.
   - Verified Phase 4 read-only rootfs and OverlayFS security protections remain fully intact.

---

## 3. Files Created or Modified

| File | Status | Description |
| :--- | :--- | :--- |
| `configs/qemu_aarch64_virt_edgeml_defconfig` | Modified | Enabled `BR2_PACKAGE_TENSORFLOW_LITE=y` |
| `ml_model/mobilenet_v2_1.0_224_quant.tflite` | Created | Quantized INT8 MobileNetV2 flatbuffer model (3.5 MB) |
| `ml_model/imagenet_labels.txt` | Created | 1001-class ImageNet synset labels file (22 KB) |
| `ml_model/mobilenet_v2_1.0_224_quant_info.txt` | Created | Model metadata summary |
| `src/edge_classify.cpp` | Created | Production edge image classification C++ CLI application |
| `src/image_loader.h` / `image_loader.cpp` | Created | PPM P6 binary and P3 ASCII image parser and writer |
| `src/image_preprocessor.h` / `image_preprocessor.cpp` | Created | Bilinear resize and dynamic input tensor quantization engine |
| `src/Makefile` | Created | Cross-compilation Makefile targeting AArch64 GCC 13.4 |
| `tests/test_image_pipeline.cpp` | Created | Unit test validating PPM parsing, comments, and bilinear resize |
| `scripts/generate_test_images.py` | Created | Test pattern generator producing binary and ASCII PPM images |
| `rootfs/usr/bin/edge-classify` | Created | Stripped AArch64 production executable (66.1 KB) |
| `rootfs/usr/share/edgeml/images/*` | Created | Bundled sample PPM test images in rootfs overlay |
| `docs/phase6_edge_inference.md` | Created | Comprehensive Phase 6 architecture, benchmark, and verification report |
| `PROJECT_STATUS.md` | Modified | Updated with Phase 6 completion, benchmarks, and verification results |

---

## 4. Commands Executed

```bash
# 1. Host unit test cross-compilation and verification
output_qemu/host/bin/aarch64-buildroot-linux-gnu-g++ -O2 -std=c++17 tests/test_image_pipeline.cpp src/image_loader.cpp src/image_preprocessor.cpp -Ioutput_qemu/staging/usr/include -Loutput_qemu/staging/usr/lib -ltensorflow-lite -o /tmp/arm_test_pipeline
qemu-aarch64 -L output_qemu/target /tmp/arm_test_pipeline

# 2. Compile and strip edge-classify binary
make -C src clean && make -C src
../output_qemu/host/bin/aarch64-buildroot-linux-gnu-strip src/edge-classify

# 3. Generate test patterns and stage binaries
python3 scripts/generate_test_images.py rootfs/usr/share/edgeml/images
cp src/edge-classify rootfs/usr/bin/edge-classify
chmod 755 rootfs/usr/bin/edge-classify && chmod 644 rootfs/usr/share/edgeml/images/*
make -C src clean

# 4. Rebuild rootfs image
./scripts/build.sh build qemu 4

# 5. Boot QEMU and execute automated 12-test suite
/tmp/phase6_run_tests.sh
```

---

## 5. Build / Test Results

### 1. Generated Build Artifacts (`output_qemu/images/` & `target/`)
- **Kernel Binary:** `output_qemu/images/Image` (Size: **12 MB**) — Linux 6.12.27 ARM64
- **Root Filesystem:** `output_qemu/images/rootfs.ext4` (Size: **60 MB**, ext4)
- **TFLite Shared Library:** `output_qemu/target/usr/lib/libtensorflow-lite.so` (Size: **3.1 MB**, stripped, ARM aarch64)
- **Edge Inference App:** `rootfs/usr/bin/edge-classify` (Size: **66.1 KB**, stripped, ARM aarch64)
- **Sample Images:** `rootfs/usr/share/edgeml/images/` (4 images, P6 and P3 PPM)

### 2. Live QEMU Verification Test Output (12 / 12 PASS)

| Test ID | Test Description | Observed Target Terminal Output | Result |
| :--- | :--- | :--- | :--- |
| **Test 1** | CLI Help Display | `/usr/bin/edge-classify --help` prints complete usage and flag documentation | **PASS** |
| **Test 2** | Defensive Error Handling | Rejects non-existent models, images, and invalid `--threads 0` | **PASS** |
| **Test 3** | Asset Verification | Confirmed `edge-classify` (66.1 KB), models, and 4 test images in target rootfs | **PASS** |
| **Test 4** | P6 Binary PPM Inference | Resizes 300x200 sample, runs inference, outputs Top-5 classes (`digital clock`, `theater curtain`) | **PASS** |
| **Test 5** | P3 ASCII PPM Inference | Ingests ASCII PPM with comments, upsamples 64x64 to 224x224, outputs Top-3 (`lampshade`, `table lamp`) | **PASS** |
| **Test 6** | Native 224x224 Image | Classifies native test pattern (`padlock`, `matchstick`, `binder`) | **PASS** |
| **Test 7** | Benchmark Mode (10 iter) | Measured 10 iterations: Avg latency **478.73 ms**, **2.09 FPS**, VmRSS 13,624 KB | **PASS** |
| **Test 8** | Multi-Thread Scaling | Demonstrated **1.87x speedup**: **471.95 ms** (1T) vs **252.07 ms** (2T, 3.97 FPS) | **PASS** |
| **Test 9** | Unprivileged User Exec | `su -s /bin/sh edgeai` runs inference successfully with zero permission errors | **PASS** |
| **Test 10**| Phase 4 Hardening Check | Rootfs read-only confirmed (`touch /test_ro_fail` denied); tmpfs & OverlayFS active; `edgeai` UID 1001 verified | **PASS** |
| **Test 11**| Phase 5 Backward Compat | Ran Phase 5 verification binary `/usr/bin/tflite_test` with identical results | **PASS** |
| **Test 12**| Clean System Teardown | `poweroff` cleanly tears down OverlayFS and issues system power down | **PASS** |

### 3. Empirical Multi-Thread Scaling Summary

```
[1 Thread]  Avg Latency: 471.95 ms | Throughput: 2.12 FPS | Active VmRSS: 13,628 KB
[2 Threads] Avg Latency: 252.07 ms | Throughput: 3.97 FPS | Active VmRSS: 13,700 KB
Speedup: 1.87x (+87.3% throughput increase, latency cut by 46.6%)
Memory Overhead: +72 KB for thread pool management; system free RAM >91 MB
```

---

## 6. Errors and Their Resolutions

1. **Issue:** When building `tests/tflite_test` and `tests/test_image_pipeline` initially with host `g++`, compilation failed with syntax errors regarding `__Float32x4_t` in `bits/math-vector.h`.  
   **Root Cause:** Host `g++` was invoked instead of the cross-compiler, and included ARM64 sysroot headers which contain AArch64 SVE/Neon vector extensions incompatible with host x86_64.  
   **Resolution:** Modified `src/Makefile` to explicitly assign `CXX = ../output_qemu/host/bin/aarch64-buildroot-linux-gnu-g++` and ran ARM64 binaries via `qemu-aarch64 -L output_qemu/target`.

---

## 7. Remaining Tasks

- [x] **Phase 0:** Workspace Inspection, Environment Verification & Architectural Foundation (COMPLETED & VERIFIED)
- [x] **Phase 1:** Requirements Analysis & Architectural Specification (COMPLETED & VERIFIED)
- [x] **Phase 2:** Buildroot Configuration and Target Preparation (COMPLETED & VERIFIED)
- [x] **Phase 3:** Build Initial ARM64 Linux System for QEMU (COMPLETED & VERIFIED)
- [x] **Phase 4:** Root Filesystem Hardening & Storage/OverlayFS Setup (COMPLETED & VERIFIED)
- [x] **Phase 5:** Machine Learning Runtime Integration (COMPLETED & VERIFIED)
- [x] **Phase 6:** Edge Inference Application (COMPLETED & VERIFIED)
- [ ] **Phase 7:** Target A Hardware Build (Build full Raspberry Pi 4 Model B kernel 6.6, BCM2711 DTB, firmware, and generate `sdcard.img`).
- [ ] **Phase 8:** Target Deployment, Verification & Comparative Benchmarking (Empirical benchmarking against Raspberry Pi OS Lite).

---

## 8. Exact Next Step

**Awaiting user approval to proceed to Phase 7:**  
Configure and build Target A for physical hardware: Raspberry Pi 4 Model B (4GB RAM) using `configs/raspberrypi4_64_edgeml_defconfig` (Linux kernel 6.6.x, Broadcom BCM2711 device tree, GPU firmware, bootloader config, and bootable `sdcard.img`).
