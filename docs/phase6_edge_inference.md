# Phase 6: Edge Inference Application (`edge-classify`)

## 1. Overview & Objective

Phase 6 delivers a native, lightweight, production C++ command-line application named **`edge-classify`** for the **Edge ML Linux OS (Track A)**. The application implements an end-to-end edge computer vision inference pipeline that:

1. **Loads real images** in Portable Pixmap (PPM) format supporting both binary (P6) and ASCII (P3) representations with comment filtering.
2. **Preprocesses image tensors** via high-precision bilinear interpolation resizing to arbitrary model geometries (e.g. $224 \times 224 \times 3$) and dynamically normalizes and quantizes input values using model tensor parameters ($\text{scale}$, $\text{zero\_point}$).
3. **Executes INT8 MobileNetV2 inference** through the statically embedded TensorFlow Lite 2.11.0 runtime with Google's ARM NEON `ruy` matrix multiplication engine.
4. **Calculates Top-$K$ predictions** by dequantizing output logits, mapping them to human-readable ImageNet labels, and printing a structured classification table.
5. **Measures runtime performance** with high-resolution monotonic clocks (`min`, `max`, `avg`, `median` latency, and throughput FPS) and reads Linux `/proc/self/status` (`VmRSS`, `VmPeak`) and `/proc/meminfo` (`MemAvailable`).
6. **Maintains security and robustness**, executing seamlessly under unprivileged user privileges (`edgeai`, UID/GID 1001) while preserving Phase 4 read-only rootfs and OverlayFS protections.

---

## 2. Pipeline Architecture

```
[ Input PPM Image (P6 / P3) ]
             │
             ▼
[ ImageLoader: PPM Parser & Header Sanitizer ]
             │
             ▼
[ ImagePreprocessor: Bilinear Interpolation Resize ]  (e.g., 300x200 -> 224x224)
             │
             ▼
[ Input Quantization: round((pixel / 127.5 - 1.0) / scale) + zero_point ]
             │
             ▼
[ TensorFlow Lite Runtime (libtensorflow-lite.so) ]
  • Configured with N worker threads
  • NEON-accelerated tensor kernels (ruy engine)
             │
             ▼
[ Top-K Output Ranker & Synset Label Decoder ]
             │
             ▼
[ Terminal Output: Predictions Table, Latencies & Memory Footprint ]
```

### Key Technical Characteristics

- **Zero Bloat Dependencies:** Uses a self-contained PPM parser to avoid linking heavy external graphic libraries (`libpng`, `libjpeg`, `OpenCV`). The stripped binary is only **66.1 KB**.
- **Quantization Compatibility:** Dynamically queries model tensor types (`UINT8`, `INT8`, `FLOAT32`) and tensor quantization metadata.
- **Thread Scalability:** Configures TFLite worker thread pool via `interpreter->SetNumThreads(N)`, delivering measurable multi-core acceleration on SMP architectures.

---

## 3. Command-Line Interface (CLI)

```
Usage: edge-classify [options]

Production edge image classification using TensorFlow Lite.

Options:
  --image <path>       Input image path (supports PPM P6 binary and P3 ASCII)
  --model <path>       TFLite model path (default: /usr/share/edgeml/models/mobilenet_v2_1.0_224_quant.tflite)
  --labels <path>      Labels file path (default: /usr/share/edgeml/models/imagenet_labels.txt)
  --threads <N>        Number of inference threads (default: 1, must be >= 1)
  --top <K>            Display top K prediction classes (default: 5, must be >= 1)
  --benchmark [N]      Run benchmark mode with N iterations (default: 10)
  --help, -h           Show this help message and exit

Examples:
  edge-classify --image /usr/share/edgeml/images/sample.ppm --top 5
  edge-classify --image /usr/share/edgeml/images/sample.ppm --threads 2 --benchmark 10
  su -s /bin/sh edgeai -c "edge-classify --image /usr/share/edgeml/images/sample.ppm --top 3"
```

---

## 4. Empirical Benchmarks (Measured in QEMU ARM64 Virt)

All measurements below were gathered on the target running the hardened Linux 6.12.27 ARM64 operating system within the constrained 128 MB RAM budget.

### 4.1. Single vs Multi-Thread Scaling (5 Iterations)

| Metric | 1 Thread (`--threads 1`) | 2 Threads (`--threads 2`) | Delta / Speedup |
| :--- | :--- | :--- | :--- |
| **Min Latency** | 469.08 ms | 250.38 ms | **1.87x faster** (46.6% reduction) |
| **Max Latency** | 477.66 ms | 254.72 ms | **1.88x faster** |
| **Avg Latency** | **471.95 ms** | **252.07 ms** | **1.87x faster** |
| **Median Latency**| 471.09 ms | 251.73 ms | **1.87x faster** |
| **Throughput (FPS)**| **2.12 inf/sec** | **3.97 inf/sec** | **+87.3% throughput** |
| **Active VmRSS** | 13,628 KB (~13.3 MB) | 13,700 KB (~13.4 MB) | +72 KB thread pool overhead |
| **System MemAvailable**| 93,784 KB (~91.6 MB) | 94,196 KB (~92.0 MB) | >91 MB RAM headroom preserved |

### 4.2. Image Ingestion & Preprocessing Breakdown

| Phase | Gradient Sample (300x200 -> 224x224) | Color Bars (Native 224x224) | ASCII Sample (64x64 -> 224x224) |
| :--- | :--- | :--- | :--- |
| **File Read Time** | 4.78 ms | 4.86 ms | 1.83 ms |
| **Resize & Quantization** | 14.46 ms (Bilinear downscale) | 3.86 ms (Direct copy) | 14.54 ms (Bilinear upscale) |
| **Input Tensor Size** | $224 \times 224 \times 3$ (150,528 B) | $224 \times 224 \times 3$ (150,528 B) | $224 \times 224 \times 3$ (150,528 B) |

---

## 5. Verification Test Suite Matrix (12 / 12 PASS)

The automated QEMU verification suite was executed live on the target kernel and rootfs:

| Test ID | Test Description | Target Command & Verification | Result |
| :--- | :--- | :--- | :--- |
| **Test 1** | CLI Help Display | `/usr/bin/edge-classify --help` prints complete usage and flag summary | **PASS** |
| **Test 2** | Defensive Error Handling | Rejects non-existent models, non-existent images, and `--threads 0` with exit code 1 | **PASS** |
| **Test 3** | Staged Assets Verification | Confirmed `/usr/bin/edge-classify` (66.1 KB), models, and 4 test images in `/usr/share/edgeml/` | **PASS** |
| **Test 4** | P6 Binary PPM Inference | Resizes 300x200 sample, runs inference, outputs Top-5 classes (`digital clock`, `theater curtain`) | **PASS** |
| **Test 5** | P3 ASCII PPM Inference | Ingests ASCII PPM with comments, upsamples 64x64 to 224x224, outputs Top-3 (`lampshade`, `table lamp`) | **PASS** |
| **Test 6** | Native 224x224 Image | Classifies native test pattern (`padlock`, `matchstick`, `binder`) | **PASS** |
| **Test 7** | Benchmark Mode (10 iter) | Measured 10 iterations: Avg latency 478.73 ms, 2.09 FPS, VmRSS 13,624 KB | **PASS** |
| **Test 8** | Multi-Thread Scaling | Demonstrated 1.87x speedup: 471.95 ms (1T) vs 252.07 ms (2T) | **PASS** |
| **Test 9** | Unprivileged User Exec | `su -s /bin/sh edgeai` successfully runs inference without root permissions | **PASS** |
| **Test 10**| Phase 4 Hardening Check | Rootfs read-only confirmed (`touch /test_ro_fail` denied); tmpfs & OverlayFS on `/etc` active; `edgeai` UID 1001 verified | **PASS** |
| **Test 11**| Backward Compatibility | Ran Phase 5 native verification binary `/usr/bin/tflite_test` with identical results | **PASS** |
| **Test 12**| Clean System Teardown | `poweroff` cleanly tears down OverlayFS, syncs storage, and issues kernel power down | **PASS** |

---

## 6. Source File Manifest

| File Path | Description |
| :--- | :--- |
| `src/edge_classify.cpp` | Main CLI application, timing logic, arg parsing, model execution, Top-K formatting |
| `src/image_loader.h` | PPM image data structure and loader definitions |
| `src/image_loader.cpp` | Robust PPM P6 (binary) and P3 (ASCII) parser with comment filtering |
| `src/image_preprocessor.h` | Image preprocessor interface |
| `src/image_preprocessor.cpp` | Bilinear resize engine and dynamic tensor quantization population |
| `src/Makefile` | Cross-compilation Makefile targeting AArch64 Buildroot toolchain |
| `tests/test_image_pipeline.cpp` | Unit test verifying PPM parsing, comments, bilinear resize, and corrupt image rejection |
| `scripts/generate_test_images.py` | Generator producing test patterns in binary and ASCII formats |
| `rootfs/usr/bin/edge-classify` | Stripped AArch64 executable installed in the rootfs overlay |
| `rootfs/usr/share/edgeml/images/*` | Staged test images bundled into `rootfs.ext4` |
