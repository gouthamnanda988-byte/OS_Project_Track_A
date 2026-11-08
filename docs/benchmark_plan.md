# Benchmark & Evaluation Plan: Track A vs Raspberry Pi OS Lite

**Project:** Application-Specific Linux Distribution for ML Inference at the Edge  
**Target:** Raspberry Pi 4 Model B (4GB RAM)  
**Baseline OS:** Raspberry Pi OS Lite (64-bit Debian Bookworm)  
**Custom OS:** Track A Custom Embedded Linux (Buildroot 2025.02.10 LTS)  
**Date:** November 2026  
**Status:** Approved (Phase 0)

---

## 1. Experimental Methodology & Rigor

To ensure scientific reproducibility and eliminate bias:
1. **Identical Hardware:** All benchmarks must run on the exact same physical Raspberry Pi 4 Model B board (4GB RAM, Revision 1.4/1.5) using the same official Raspberry Pi 15.3W USB-C power supply.
2. **Identical Storage Media:** Use identical SanDisk Ultra 32GB Class 10 / A1 MicroSD cards formatted before flashing.
3. **Thermal Stabilization:** A 3-minute idle cool-down period is enforced prior to every benchmark run to eliminate thermal throttling artifacts. Ambient room temperature recorded.
4. **No Fabricated Data:** Every single data point must be collected directly from physical hardware or simulated QEMU targets, tagged with execution timestamps and raw output logs stored in `output/benchmarks/`.

---

## 2. Benchmark Metrics & Instrumentation

### 2.1 Boot Latency Breakdown

| Phase | Measurement Technique | Custom OS Target | Baseline Target (RPi OS) |
| :--- | :--- | :--- | :--- |
| **Firmware to Kernel Init** | VideoCore firmware timestamp (`dmesg [ 0.000000]`) | Recorded | Recorded |
| **Kernel Boot Time** | `dmesg` timestamp at userspace transition (`Run /sbin/init`) | < 2.0 s | ~4.5 - 6.5 s |
| **Userspace to App Start** | Sysinit script timestamp (`S99edge-inference` start) | < 1.0 s | ~12.0 - 18.0 s (`systemd`) |
| **Cold Boot to 1st Inference** | Wall-clock time from power-on to completion of 1st inference | < 5.0 s | ~25.0 - 40.0 s |

**Instrumentation Tool:**  
- Serial console high-resolution logging (`picocom` / `minicom` with millisecond timestamps).
- Alternative hardware probe: GPIO pin toggle at start of bootloader (`bootcode`), kernel init, and application completion, monitored via logic analyzer.

### 2.2 Storage & Memory Footprint

| Metric | Measurement Command | Unit | Custom OS Target |
| :--- | :--- | :--- | :--- |
| **Flash Image Size** | `ls -lh sdcard.img` | Megabytes (MB) | < 250 MB |
| **RootFS Partition Used** | `df -m /` | Megabytes (MB) | < 150 MB |
| **Idle System RAM** | `cat /proc/meminfo` (`MemTotal - MemAvailable`) | Megabytes (MB) | < 40 MB |
| **Active App Resident RAM** | `grep VmRSS /proc/<pid>/status` | Megabytes (MB) | < 30 MB |
| **Active App Virtual RAM** | `grep VmSize /proc/<pid>/status` | Megabytes (MB) | < 80 MB |

### 2.3 Machine Learning Inference Performance

- **Target Model:** MobileNetV2 (INT8 quantized, input tensor $1 \times 224 \times 224 \times 3$, flatbuffer size ~3.4 MB).
- **Execution Engine:** TensorFlow Lite with XNNPACK ARM NEON acceleration.
- **Iterations:** 10 warm-up runs followed by 100 benchmark iterations.
- **Clock Resolution:** `clock_gettime(CLOCK_MONOTONIC)` with nanosecond precision.

Metrics captured per run:
- Latency Mean ($\mu$)
- Latency Standard Deviation ($\sigma$)
- Latency Percentiles: $p_{50}$ (median), $p_{90}$, $p_{95}$, $p_{99}$
- Throughput (Frames Per Second = $1000 / \text{mean\_ms}$)

### 2.4 Thermal & Power Stability

- **Idle Temperature:** Read `/sys/class/thermal/thermal_zone0/temp` before test.
- **Sustained Load Temperature:** Run continuous inference loop for 10 minutes; log thermal progression every 5 seconds.
- **Throttling Verification:** Monitor `vcgencmd get_throttled` for undervoltage (`0x1`), frequency capping (`0x2`), or thermal throttling (`0x4`).

### 2.5 Filesystem Resilience (Abrupt Power Cut)

- **Test Protocol:** Trigger 10 unscheduled power cuts while writing simulated log data to `/data` and attempting writes to read-only `/`.
- **Validation:** Verify zero kernel panics, zero filesystem corruption on `/`, and successful boot-to-inference on subsequent power-up.

---

## 3. Data Collection Procedure

```bash
# Example benchmark execution on target:
/usr/bin/edge-classify \
    --model=/usr/share/models/mobilenet_v2_1.0_224_quant.tflite \
    --input=/usr/share/models/test_image_224.bmp \
    --threads=4 \
    --runs=100 \
    --warmup=10 \
    --output_json=/data/logs/benchmark_results.json
```

Raw logs, system telemetry, and comparative charts will be compiled directly into `output/benchmarks/benchmark_report.md`.
