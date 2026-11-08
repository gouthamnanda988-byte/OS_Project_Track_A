#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <chrono>
#include <algorithm>
#include <numeric>
#include <cmath>
#include <iomanip>
#include <memory>
#include <unistd.h>

#include "image_loader.h"
#include "image_preprocessor.h"

#include "tensorflow/lite/interpreter.h"
#include "tensorflow/lite/kernels/register.h"
#include "tensorflow/lite/model.h"

// Memory reporting helpers
static long get_proc_status_kb(const std::string& field) {
    std::ifstream file("/proc/self/status");
    if (!file.is_open()) return -1;
    std::string line;
    while (std::getline(file, line)) {
        if (line.rfind(field, 0) == 0) {
            std::istringstream iss(line);
            std::string name;
            long value = 0;
            iss >> name >> value;
            return value;
        }
    }
    return -1;
}

static long get_meminfo_kb(const std::string& field) {
    std::ifstream file("/proc/meminfo");
    if (!file.is_open()) return -1;
    std::string line;
    while (std::getline(file, line)) {
        if (line.rfind(field, 0) == 0) {
            std::istringstream iss(line);
            std::string name;
            long value = 0;
            iss >> name >> value;
            return value;
        }
    }
    return -1;
}

static std::vector<std::string> load_labels(const std::string& labels_path) {
    std::vector<std::string> labels;
    if (labels_path.empty()) return labels;
    std::ifstream file(labels_path);
    if (!file.is_open()) return labels;
    std::string line;
    while (std::getline(file, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        labels.push_back(line);
    }
    return labels;
}

static void print_usage(const char* prog_name) {
    std::cout << "Usage: " << prog_name << " [options]\n\n"
              << "Production edge image classification using TensorFlow Lite.\n\n"
              << "Options:\n"
              << "  --image <path>       Input image path (supports PPM P6 binary and P3 ASCII)\n"
              << "  --model <path>       TFLite model path (default: /usr/share/edgeml/models/mobilenet_v2_1.0_224_quant.tflite)\n"
              << "  --labels <path>      Labels file path (default: /usr/share/edgeml/models/imagenet_labels.txt)\n"
              << "  --threads <N>        Number of inference threads (default: 1)\n"
              << "  --top <K>            Display top K prediction classes (default: 5)\n"
              << "  --benchmark [N]      Run benchmark mode with N iterations (default: 10)\n"
              << "  --help, -h           Show this help message and exit\n\n"
              << "Example:\n"
              << "  " << prog_name << " --image /usr/share/edgeml/images/sample.ppm --top 5\n"
              << "  " << prog_name << " --image /usr/share/edgeml/images/sample.ppm --threads 2 --benchmark 15\n";
}

int main(int argc, char* argv[]) {
    std::string model_path = "/usr/share/edgeml/models/mobilenet_v2_1.0_224_quant.tflite";
    std::string labels_path = "/usr/share/edgeml/models/imagenet_labels.txt";
    std::string image_path = "";
    int num_threads = 1;
    int top_k = 5;
    bool benchmark_mode = false;
    int benchmark_iterations = 10;

    // Parse command line arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--help" || arg == "-h") {
            print_usage(argv[0]);
            return 0;
        } else if (arg == "--model" && i + 1 < argc) {
            model_path = argv[++i];
        } else if (arg == "--labels" && i + 1 < argc) {
            labels_path = argv[++i];
        } else if (arg == "--image" && i + 1 < argc) {
            image_path = argv[++i];
        } else if (arg == "--threads" && i + 1 < argc) {
            int val = std::atoi(argv[++i]);
            if (val <= 0) {
                std::cerr << "[ERROR] Invalid thread count: " << val << " (must be >= 1)\n";
                return 1;
            }
            num_threads = val;
        } else if (arg == "--top" && i + 1 < argc) {
            int val = std::atoi(argv[++i]);
            if (val <= 0) {
                std::cerr << "[ERROR] Invalid top-K value: " << val << " (must be >= 1)\n";
                return 1;
            }
            top_k = val;
        } else if (arg == "--benchmark") {
            benchmark_mode = true;
            if (i + 1 < argc && argv[i + 1][0] != '-') {
                benchmark_iterations = std::max(1, std::atoi(argv[++i]));
            }
        } else {
            std::cerr << "[ERROR] Unknown or incomplete argument: " << arg << "\n";
            print_usage(argv[0]);
            return 1;
        }
    }

    std::cout << "==================================================================\n";
    std::cout << "  Edge ML Linux OS (Track A) — Production Edge Classifier\n";
    std::cout << "==================================================================\n";

    if (image_path.empty() && !benchmark_mode) {
        std::cerr << "[ERROR] No input image specified. Use --image <path> or --benchmark.\n";
        print_usage(argv[0]);
        return 1;
    }

    // 1. Load Model FlatBuffer
    std::cout << "[INFO] Loading model: " << model_path << "\n";
    std::unique_ptr<tflite::FlatBufferModel> model =
        tflite::FlatBufferModel::BuildFromFile(model_path.c_str());
    if (!model) {
        std::cerr << "[ERROR] Failed to load model from: " << model_path << "\n";
        return 2;
    }
    std::cout << "[PASS] Model FlatBuffer loaded successfully.\n";

    // 2. Build TFLite Interpreter
    tflite::ops::builtin::BuiltinOpResolver resolver;
    tflite::InterpreterBuilder builder(*model, resolver);
    std::unique_ptr<tflite::Interpreter> interpreter;
    if (builder(&interpreter) != kTfLiteOk || !interpreter) {
        std::cerr << "[ERROR] Failed to instantiate TFLite interpreter.\n";
        return 3;
    }

    if (interpreter->SetNumThreads(num_threads) != kTfLiteOk) {
        std::cerr << "[WARN] Failed to set thread count to " << num_threads << ", proceeding with default.\n";
    } else {
        std::cout << "[PASS] Interpreter configured with " << num_threads << " thread(s).\n";
    }

    // 3. Allocate Tensors
    if (interpreter->AllocateTensors() != kTfLiteOk) {
        std::cerr << "[ERROR] Failed to allocate tensor memory.\n";
        return 4;
    }
    std::cout << "[PASS] Tensor allocation complete.\n";

    // 4. Validate Input & Output Tensors
    int input_idx = interpreter->inputs()[0];
    int output_idx = interpreter->outputs()[0];
    TfLiteTensor* input_tensor = interpreter->tensor(input_idx);
    TfLiteTensor* output_tensor = interpreter->tensor(output_idx);

    if (!input_tensor || !output_tensor) {
        std::cerr << "[ERROR] Missing input or output tensor references.\n";
        return 5;
    }

    int target_h = (input_tensor->dims->size >= 3) ? input_tensor->dims->data[1] : 224;
    int target_w = (input_tensor->dims->size >= 3) ? input_tensor->dims->data[2] : 224;
    int target_c = (input_tensor->dims->size >= 4) ? input_tensor->dims->data[3] : 3;

    std::cout << "[INFO] Input tensor:  [" << target_w << "x" << target_h << "x" << target_c
              << "], type: " << TfLiteTypeGetName(input_tensor->type)
              << ", scale=" << input_tensor->params.scale
              << ", zero_point=" << input_tensor->params.zero_point << "\n";

    int out_elements = 1;
    for (int i = 0; i < output_tensor->dims->size; ++i) {
        out_elements *= output_tensor->dims->data[i];
    }
    std::cout << "[INFO] Output tensor: [" << out_elements << " classes], type: "
              << TfLiteTypeGetName(output_tensor->type)
              << ", scale=" << output_tensor->params.scale
              << ", zero_point=" << output_tensor->params.zero_point << "\n";

    // 5. Image Ingestion and Preprocessing
    ImageRGB raw_image;
    double load_time_ms = 0.0;
    double prep_time_ms = 0.0;

    if (!image_path.empty()) {
        std::cout << "\n--- Image Ingestion & Preprocessing ---\n";
        std::cout << "[INFO] Loading image: " << image_path << "\n";
        auto t0 = std::chrono::steady_clock::now();
        std::string err;
        if (!ImageLoader::load_ppm(image_path, raw_image, err)) {
            std::cerr << "[ERROR] Image loading failed: " << err << "\n";
            return 6;
        }
        auto t1 = std::chrono::steady_clock::now();
        load_time_ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
        std::cout << "[PASS] Image loaded: " << raw_image.width << "x" << raw_image.height
                  << " (RGB) in " << std::fixed << std::setprecision(2) << load_time_ms << " ms\n";

        auto t2 = std::chrono::steady_clock::now();
        ImageRGB resized = ImagePreprocessor::resize_bilinear(raw_image, target_w, target_h);
        if (!ImagePreprocessor::populate_input_tensor(resized, input_tensor, err)) {
            std::cerr << "[ERROR] Image preprocessing failed: " << err << "\n";
            return 7;
        }
        auto t3 = std::chrono::steady_clock::now();
        prep_time_ms = std::chrono::duration<double, std::milli>(t3 - t2).count();
        std::cout << "[PASS] Resized to " << target_w << "x" << target_h
                  << " and populated input tensor in " << prep_time_ms << " ms\n";
    } else {
        std::cout << "\n[INFO] Benchmark mode without image: populating input tensor with synthetic test pattern...\n";
        ImageRGB test_pattern = ImageLoader::create_test_pattern(target_w, target_h);
        std::string err;
        ImagePreprocessor::populate_input_tensor(test_pattern, input_tensor, err);
    }

    // 6. Warmup Inference
    std::cout << "\n--- Model Inference ---\n";
    std::cout << "[INFO] Executing warm-up invocation...\n";
    if (interpreter->Invoke() != kTfLiteOk) {
        std::cerr << "[ERROR] Warm-up inference failed.\n";
        return 8;
    }

    // 7. Benchmark Mode / Single Inference
    int iterations = benchmark_mode ? benchmark_iterations : 1;
    std::vector<double> latencies;
    latencies.reserve(iterations);

    for (int iter = 0; iter < iterations; ++iter) {
        auto start = std::chrono::steady_clock::now();
        TfLiteStatus status = interpreter->Invoke();
        auto end = std::chrono::steady_clock::now();

        if (status != kTfLiteOk) {
            std::cerr << "[ERROR] Inference failed on iteration " << (iter + 1) << "\n";
            return 9;
        }

        double ms = std::chrono::duration<double, std::milli>(end - start).count();
        latencies.push_back(ms);
    }

    // 8. Performance Reporting
    std::sort(latencies.begin(), latencies.end());
    double min_lat = latencies.front();
    double max_lat = latencies.back();
    double sum = std::accumulate(latencies.begin(), latencies.end(), 0.0);
    double avg_lat = sum / latencies.size();
    double median_lat = (latencies.size() % 2 == 0)
        ? (latencies[latencies.size() / 2 - 1] + latencies[latencies.size() / 2]) / 2.0
        : latencies[latencies.size() / 2];
    double throughput = (avg_lat > 0.0) ? (1000.0 / avg_lat) : 0.0;

    std::cout << "\n--- Inference Performance Metrics ---\n";
    std::cout << "[METRIC] Iterations:  " << iterations << "\n";
    std::cout << "[METRIC] Min Latency: " << std::fixed << std::setprecision(2) << min_lat << " ms\n";
    std::cout << "[METRIC] Max Latency: " << max_lat << " ms\n";
    std::cout << "[METRIC] Avg Latency: " << avg_lat << " ms\n";
    std::cout << "[METRIC] Median Lat.: " << median_lat << " ms\n";
    std::cout << "[METRIC] Throughput:  " << throughput << " inf/sec (FPS)\n";

    // 9. Memory Footprint Reporting
    std::cout << "\n--- Memory Footprint Metrics ---\n";
    long vmrss = get_proc_status_kb("VmRSS:");
    long vmpeak = get_proc_status_kb("VmPeak:");
    long memavail = get_meminfo_kb("MemAvailable:");
    std::cout << "[MEM] Process VmRSS (Active Resident): " << (vmrss >= 0 ? std::to_string(vmrss) + " KB" : "Unavailable") << "\n";
    std::cout << "[MEM] Process VmPeak (Peak Virtual):   " << (vmpeak >= 0 ? std::to_string(vmpeak) + " KB" : "Unavailable") << "\n";
    std::cout << "[MEM] System MemAvailable:             " << (memavail >= 0 ? std::to_string(memavail) + " KB" : "Unavailable") << "\n";

    // 10. Top-K Classification Output
    std::cout << "\n--- Top-" << top_k << " Classification Predictions ---\n";
    std::vector<std::string> labels = load_labels(labels_path);
    std::vector<std::pair<float, int>> class_scores;
    class_scores.reserve(out_elements);

    float out_scale = output_tensor->params.scale;
    int out_zp = output_tensor->params.zero_point;

    if (output_tensor->type == kTfLiteUInt8) {
        uint8_t* out_data = interpreter->typed_tensor<uint8_t>(output_idx);
        for (int i = 0; i < out_elements; ++i) {
            float dequant = (out_scale > 0.0f) ? (static_cast<int>(out_data[i]) - out_zp) * out_scale : out_data[i];
            class_scores.push_back({dequant, i});
        }
    } else if (output_tensor->type == kTfLiteInt8) {
        int8_t* out_data = interpreter->typed_tensor<int8_t>(output_idx);
        for (int i = 0; i < out_elements; ++i) {
            float dequant = (out_scale > 0.0f) ? (static_cast<int>(out_data[i]) - out_zp) * out_scale : out_data[i];
            class_scores.push_back({dequant, i});
        }
    } else if (output_tensor->type == kTfLiteFloat32) {
        float* out_data = interpreter->typed_tensor<float>(output_idx);
        for (int i = 0; i < out_elements; ++i) {
            class_scores.push_back({out_data[i], i});
        }
    }

    std::sort(class_scores.begin(), class_scores.end(), [](const auto& a, const auto& b) {
        return a.first > b.first;
    });

    std::cout << " Note: Scores represent dequantized logits / activation confidence.\n";
    std::cout << " +------+----------+------------+------------------------------------------------------------\n";
    std::cout << " | Rank | Class ID | Confidence | Label\n";
    std::cout << " +------+----------+------------+------------------------------------------------------------\n";

    int display_count = std::min(top_k, static_cast<int>(class_scores.size()));
    for (int r = 0; r < display_count; ++r) {
        int cls_id = class_scores[r].second;
        float score = class_scores[r].first;
        std::string label = (cls_id < static_cast<int>(labels.size())) ? labels[cls_id] : "Unknown";
        std::cout << " | " << std::setw(4) << (r + 1)
                  << " | " << std::setw(8) << cls_id
                  << " | " << std::setw(10) << std::fixed << std::setprecision(4) << score
                  << " | " << label << "\n";
    }
    std::cout << " +------+----------+------------+------------------------------------------------------------\n";

    std::cout << "\n[PASS] Edge classification executed successfully.\n";
    std::cout << "==================================================================\n";
    return 0;
}
