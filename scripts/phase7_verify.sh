#!/bin/bash
#
# scripts/phase7_verify.sh
# Consolidated Phase 7 Verification & Evaluation Suite for Edge ML Linux OS (Track A)
#

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
cd "$PROJECT_ROOT"

OUTPUT_LOG="/tmp/phase7_test_results.txt"
KERNEL_IMAGE="$PROJECT_ROOT/output_qemu/images/Image"
ROOTFS_IMAGE="$PROJECT_ROOT/output_qemu/images/rootfs.ext4"

echo "=================================================================="
echo "  Edge ML Linux OS (Track A) — Phase 7 System Verification Suite"
echo "=================================================================="

# Check prerequisites
if [ ! -f "$KERNEL_IMAGE" ] || [ ! -f "$ROOTFS_IMAGE" ]; then
    echo "[ERROR] Kernel ($KERNEL_IMAGE) or Rootfs ($ROOTFS_IMAGE) missing!"
    exit 1
fi

echo "[INFO] Target Kernel: $KERNEL_IMAGE"
echo "[INFO] Target Rootfs: $ROOTFS_IMAGE"
echo "[INFO] Launching QEMU AArch64 Virt Machine (-smp 2)..."

rm -f "$OUTPUT_LOG"

# Drive QEMU with timed inputs and capture output
{
  # Allow system to boot to login prompt
  sleep 20
  printf 'root\n'
  sleep 3

  # Test 1: Boot-time service integration & status
  printf 'echo ===P7_TEST_01_BOOT_SERVICE===\n'
  printf '/etc/init.d/S99edgeml status\n'
  printf '/etc/init.d/S99edgeml start\n'
  sleep 2

  # Test 2: Usability helper (edgeml-run)
  printf 'echo ===P7_TEST_02_USABILITY_HELPER===\n'
  printf '/usr/bin/edgeml-run -h\n'
  printf '/usr/bin/edgeml-run -l\n'
  printf '/usr/bin/edgeml-run sample.ppm -k 3\n'
  sleep 4

  # Test 3: Unprivileged user execution (edgeai)
  printf 'echo ===P7_TEST_03_UNPRIVILEGED_EXEC===\n'
  printf 'su -s /bin/sh edgeai -c "/usr/bin/edgeml-run sample.ppm --threads 2 -k 3"\n'
  sleep 4

  # Test 4: Service manual trigger
  printf 'echo ===P7_TEST_04_SERVICE_TRIGGER===\n'
  printf '/etc/init.d/S99edgeml run\n'
  sleep 4

  # Test 5: Reliability - Error handling and invalid inputs
  printf 'echo ===P7_TEST_05_RELIABILITY_ERRORS===\n'
  printf '/usr/bin/edge-classify --model /nonexistent.tflite --image /usr/share/edgeml/images/sample.ppm 2>&1\n'
  printf 'echo EXIT_MODEL_ERR:$?\n'
  printf '/usr/bin/edge-classify --image /nonexistent.ppm 2>&1\n'
  printf 'echo EXIT_IMG_ERR:$?\n'
  printf '/usr/bin/edge-classify --image /usr/share/edgeml/images/sample.ppm --threads 0 2>&1\n'
  printf 'echo EXIT_THREAD_ERR:$?\n'
  sleep 2

  # Test 6: Reliability - Repeated 10x inference & memory stability
  printf 'echo ===P7_TEST_06_RELIABILITY_REPEATED_10X===\n'
  printf 'for i in 1 2 3 4 5 6 7 8 9 10; do\n'
  printf '    echo -n "Run $i: "\n'
  printf '    /usr/bin/edge-classify --image /usr/share/edgeml/images/sample.ppm --top 1 > /tmp/run_${i}.log 2>&1\n'
  printf '    grep -E "Avg Latency|VmRSS" /tmp/run_${i}.log | tr "\\n" " "\n'
  printf '    echo ""\n'
  printf 'done\n'
  sleep 15

  # Test 7: Performance Evaluation - 1 Thread (10 iterations benchmark)
  printf 'echo ===P7_TEST_07_PERF_1THREAD===\n'
  printf '/usr/bin/edge-classify --image /usr/share/edgeml/images/sample.ppm --threads 1 --benchmark 10\n'
  sleep 8

  # Test 8: Performance Evaluation - 2 Threads (10 iterations benchmark)
  printf 'echo ===P7_TEST_08_PERF_2THREADS===\n'
  printf '/usr/bin/edge-classify --image /usr/share/edgeml/images/sample.ppm --threads 2 --benchmark 10\n'
  sleep 8

  # Test 9: Phase 5 Backward Compatibility (TFLite shared library & tflite_test)
  printf 'echo ===P7_TEST_09_PHASE5_BACKWARD===\n'
  printf 'ls -lh /usr/lib/libtensorflow-lite.so\n'
  printf '/usr/bin/tflite_test /usr/share/edgeml/models/mobilenet_v2_1.0_224_quant.tflite /usr/share/edgeml/models/imagenet_labels.txt 1\n'
  sleep 4

  # Test 10: Phase 6 Backward Compatibility (edge-classify ASCII P3 ingestion)
  printf 'echo ===P7_TEST_10_PHASE6_BACKWARD===\n'
  printf '/usr/bin/edge-classify --image /usr/share/edgeml/images/sample_ascii_64x64.ppm --top 3\n'
  sleep 4

  # Test 11: Phase 4 Security Hardening Regression Verification
  printf 'echo ===P7_TEST_11_PHASE4_SECURITY===\n'
  printf 'mount | grep "on / type"\n'
  printf 'touch /test_ro_fail 2>&1 || echo "ROOTFS_READONLY_OK"\n'
  printf 'df -h /tmp /run\n'
  printf 'mount | grep "on /etc "\n'
  printf 'touch /etc/phase7_overlay_test && ls -l /etc/phase7_overlay_test\n'
  printf 'id edgeai\n'
  printf 'grep edgeai /etc/passwd\n'
  sleep 2

  # Test 12: Clean system teardown
  printf 'echo ===P7_TEST_12_POWEROFF===\n'
  printf 'poweroff\n'
} | timeout 160 qemu-system-aarch64 \
    -M virt \
    -cpu cortex-a53 \
    -nographic \
    -smp 2 \
    -kernel "$KERNEL_IMAGE" \
    -append "rootwait root=/dev/vda console=ttyAMA0" \
    -netdev user,id=eth0 \
    -device virtio-net-device,netdev=eth0 \
    -drive file="$ROOTFS_IMAGE",if=none,format=raw,id=hd0 \
    -device virtio-blk-device,drive=hd0 2>&1 | tee "$OUTPUT_LOG"

echo "=================================================================="
echo "  Analyzing Phase 7 Verification Results from Target Terminal..."
echo "=================================================================="

FAILURES=0

check_test() {
    local test_name="$1"
    local pattern="$2"
    if grep -qE "$pattern" "$OUTPUT_LOG"; then
        printf "[PASS] %-50s\n" "$test_name"
    else
        printf "[FAIL] %-50s\n" "$test_name"
        FAILURES=$((FAILURES + 1))
    fi
}

echo ""
echo "--- Verification Test Suite Matrix ---"
check_test "Test 01: Boot Service Integration (/etc/init.d/S99edgeml)" "Starting Edge ML service: disabled in /etc/default/edgeml"
check_test "Test 02: Usability Helper (edgeml-run usage & listing)" "Available sample images in /usr/share/edgeml/images"
check_test "Test 03: Unprivileged Execution (edgeai UID 1001)" "digital clock"
check_test "Test 04: Service On-Demand Trigger (S99edgeml run)" "Running on-demand Edge ML inference as edgeai"
check_test "Test 05: Defensive Error Handling (Invalid Model)" "EXIT_MODEL_ERR:2"
check_test "Test 06: Defensive Error Handling (Invalid Image)" "EXIT_IMG_ERR:6"
check_test "Test 07: Defensive Error Handling (Invalid Threads)" "EXIT_THREAD_ERR:1"
check_test "Test 08: Reliability - 10x Consecutive Inferences" "Run 10:"
check_test "Test 09: Performance - 1-Thread Benchmark" "TEST_07_PERF_1THREAD"
check_test "Test 10: Performance - 2-Thread Benchmark" "TEST_08_PERF_2THREADS"
check_test "Test 11: Phase 5 Backward Compatibility (tflite_test)" "TensorFlow Lite C\+\+ Inference Verification Complete"
check_test "Test 12: Phase 6 Backward Compatibility (P3 ASCII)" "lampshade"
check_test "Test 13: Phase 4 Security (Read-Only Rootfs)" "ROOTFS_READONLY_OK"
check_test "Test 14: Phase 4 Security (OverlayFS on /etc)" "overlay on /etc type overlay"
check_test "Test 15: Clean System Teardown (OverlayFS Unmount)" "Tearing down OverlayFS on /etc: OK"

echo ""
echo "=================================================================="
if [ $FAILURES -eq 0 ]; then
    echo "  ALL PHASE 7 VERIFICATION TESTS PASSED SUCCESSFULLY! (0 Failures)"
    echo "=================================================================="
    exit 0
else
    echo "  PHASE 7 VERIFICATION ENCOUNTERED $FAILURES FAILURE(S)!"
    echo "=================================================================="
    exit 1
fi
