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
#include <unistd.h>

#include "tensorflow/lite/interpreter.h"
#include "tensorflow/lite/kernels/register.h"
#include "tensorflow/lite/model.h"

// Helper: read specific field in KB from /proc/self/status
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

// Helper: read system MemAvailable from /proc/meminfo in KB
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

// Helper: load labels from text file
static std::vector<std::string> load_labels(const std::string& labels_path) {
    std::vector<std::string> labels;
    if (labels_path.empty()) return labels;
    std::ifstream file(labels_path);
    if (!file.is_open()) {
        std::cerr << "[WARN] Could not open labels file: " << labels_path << std::endl;
        return labels;
    }
    std::string line;
    while (std::getline(file, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        labels.push_back(line);
    }
    return labels;
}

int main(int argc, char* argv[]) {
    std::cout << "========================================================\n";
    std::cout << "  Edge ML Linux OS (Track A) — TensorFlow Lite Runtime\n";
    std::cout << "========================================================\n";

    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <model.tflite> [labels.txt] [iterations]\n";
        return 1;
    }

    std::string model_path = argv[1];
    std::string labels_path = (argc >= 3) ? argv[2] : "";
    int iterations = (argc >= 4) ? std::max(1, std::stoi(argv[3])) : 5;

    std::cout << "[INFO] Initial Process VmRSS:       " << get_proc_status_kb("VmRSS:") << " KB\n";
    std::cout << "[INFO] Initial System MemAvailable: " << get_meminfo_kb("MemAvailable:") << " KB\n";
    std::cout << "[INFO] Loading model: " << model_path << "\n";

    // 1. Load model FlatBuffer
    std::unique_ptr<tflite::FlatBufferModel> model =
        tflite::FlatBufferModel::BuildFromFile(model_path.c_str());
    if (!model) {
        std::cerr << "[FAIL] Failed to load FlatBuffer model from " << model_path << "\n";
        return 2;
    }
    std::cout << "[PASS] Model FlatBuffer loaded successfully.\n";

    // 2. Build interpreter with BuiltinOpResolver
    tflite::ops::builtin::BuiltinOpResolver resolver;
    tflite::InterpreterBuilder builder(*model, resolver);
    std::unique_ptr<tflite::Interpreter> interpreter;
    if (builder(&interpreter) != kTfLiteOk || !interpreter) {
        std::cerr << "[FAIL] Failed to build TFLite interpreter.\n";
        return 3;
    }
    std::cout << "[PASS] TFLite Interpreter instantiated.\n";

    // 3. Allocate tensor buffers
    if (interpreter->AllocateTensors() != kTfLiteOk) {
        std::cerr << "[FAIL] Failed to allocate tensors.\n";
        return 4;
    }
    std::cout << "[PASS] AllocateTensors() succeeded.\n";

    // 4. Inspect Input Tensors
    const std::vector<int>& inputs = interpreter->inputs();
    std::cout << "\n--- Input Tensor Metadata ---\n";
    std::cout << "[INFO] Number of inputs: " << inputs.size() << "\n";
    if (inputs.empty()) {
        std::cerr << "[FAIL] Model has no input tensors.\n";
        return 5;
    }

    int input_idx = inputs[0];
    TfLiteTensor* input_tensor = interpreter->tensor(input_idx);
    TfLiteIntArray* in_dims = input_tensor->dims;
    std::cout << "[INFO] Input tensor [0] name: " << (input_tensor->name ? input_tensor->name : "unnamed") << "\n";
    std::cout << "[INFO] Input tensor type:     " << TfLiteTypeGetName(input_tensor->type) << "\n";
    std::cout << "[INFO] Input shape:           [";
    int in_elements = 1;
    for (int i = 0; i < in_dims->size; ++i) {
        std::cout << in_dims->data[i] << (i + 1 < in_dims->size ? ", " : "");
        in_elements *= in_dims->data[i];
    }
    std::cout << "] (" << in_elements << " elements)\n";
    std::cout << "[INFO] Input quantization:    scale=" << input_tensor->params.scale
              << ", zero_point=" << input_tensor->params.zero_point << "\n";

    // 5. Inspect Output Tensors
    const std::vector<int>& outputs = interpreter->outputs();
    std::cout << "\n--- Output Tensor Metadata ---\n";
    std::cout << "[INFO] Number of outputs: " << outputs.size() << "\n";
    if (outputs.empty()) {
        std::cerr << "[FAIL] Model has no output tensors.\n";
        return 6;
    }

    int output_idx = outputs[0];
    TfLiteTensor* output_tensor = interpreter->tensor(output_idx);
    TfLiteIntArray* out_dims = output_tensor->dims;
    std::cout << "[INFO] Output tensor [0] name: " << (output_tensor->name ? output_tensor->name : "unnamed") << "\n";
    std::cout << "[INFO] Output tensor type:      " << TfLiteTypeGetName(output_tensor->type) << "\n";
    std::cout << "[INFO] Output shape:            [";
    int out_elements = 1;
    for (int i = 0; i < out_dims->size; ++i) {
        std::cout << out_dims->data[i] << (i + 1 < out_dims->size ? ", " : "");
        out_elements *= out_dims->data[i];
    }
    std::cout << "] (" << out_elements << " elements)\n";
    std::cout << "[INFO] Output quantization:     scale=" << output_tensor->params.scale
              << ", zero_point=" << output_tensor->params.zero_point << "\n";

    // 6. Populate Input with Deterministic Synthetic Test Pattern
    std::cout << "\n--- Preparing Input Data ---\n";
    std::cout << "[INFO] Populating input tensor with synthetic test pattern (gradient signal)...\n";
    if (input_tensor->type == kTfLiteUInt8) {
        uint8_t* in_ptr = interpreter->typed_tensor<uint8_t>(input_idx);
        for (int i = 0; i < in_elements; ++i) {
            in_ptr[i] = static_cast<uint8_t>((i * 7 + 13) % 256);
        }
    } else if (input_tensor->type == kTfLiteInt8) {
        int8_t* in_ptr = interpreter->typed_tensor<int8_t>(input_idx);
        for (int i = 0; i < in_elements; ++i) {
            in_ptr[i] = static_cast<int8_t>(((i * 7 + 13) % 256) - 128);
        }
    } else if (input_tensor->type == kTfLiteFloat32) {
        float* in_ptr = interpreter->typed_tensor<float>(input_idx);
        for (int i = 0; i < in_elements; ++i) {
            in_ptr[i] = static_cast<float>((i % 256) / 255.0f);
        }
    } else {
        std::cerr << "[WARN] Unhandled input type for synthetic pattern: "
                  << TfLiteTypeGetName(input_tensor->type) << "\n";
    }

    // 7. Warmup Run
    std::cout << "\n--- Running Inference ---\n";
    std::cout << "[INFO] Warmup invocation...\n";
    if (interpreter->Invoke() != kTfLiteOk) {
        std::cerr << "[FAIL] Warmup Invoke() failed.\n";
        return 7;
    }

    // 8. Timed Benchmark Iterations
    std::cout << "[INFO] Executing " << iterations << " benchmark iterations...\n";
    std::vector<double> latencies_ms;
    latencies_ms.reserve(iterations);

    for (int iter = 0; iter < iterations; ++iter) {
        auto start = std::chrono::high_resolution_clock::now();
        TfLiteStatus status = interpreter->Invoke();
        auto end = std::chrono::high_resolution_clock::now();

        if (status != kTfLiteOk) {
            std::cerr << "[FAIL] Invoke() failed on iteration " << iter << "\n";
            return 8;
        }

        std::chrono::duration<double, std::milli> duration = end - start;
        latencies_ms.push_back(duration.count());
    }

    // 9. Latency Statistics
    double sum = std::accumulate(latencies_ms.begin(), latencies_ms.end(), 0.0);
    double avg = sum / latencies_ms.size();
    double min_lat = *std::min_element(latencies_ms.begin(), latencies_ms.end());
    double max_lat = *std::max_element(latencies_ms.begin(), latencies_ms.end());

    std::cout << "\n--- Measured Latency Statistics ---\n";
    std::cout << "[RESULT] Iterations:   " << iterations << "\n";
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "[RESULT] Min Latency:  " << min_lat << " ms\n";
    std::cout << "[RESULT] Max Latency:  " << max_lat << " ms\n";
    std::cout << "[RESULT] Avg Latency:  " << avg << " ms\n";

    // 10. Memory Usage Inspection
    std::cout << "\n--- Memory Footprint Measurement ---\n";
    long final_vmrss = get_proc_status_kb("VmRSS:");
    long final_vmpeak = get_proc_status_kb("VmPeak:");
    long final_memavail = get_meminfo_kb("MemAvailable:");
    std::cout << "[RESULT] Process VmRSS (Active RAM):    " << final_vmrss << " KB\n";
    std::cout << "[RESULT] Process VmPeak (Virtual Peak):  " << final_vmpeak << " KB\n";
    std::cout << "[RESULT] System MemAvailable:           " << final_memavail << " KB\n";

    // 11. Decode Model Predictions
    std::cout << "\n--- Output Predictions (Top 5 Classes) ---\n";
    std::vector<std::string> labels = load_labels(labels_path);
    std::vector<std::pair<float, int>> scores;
    scores.reserve(out_elements);

    if (output_tensor->type == kTfLiteUInt8) {
        uint8_t* out_ptr = interpreter->typed_tensor<uint8_t>(output_idx);
        float scale = output_tensor->params.scale;
        int zero_point = output_tensor->params.zero_point;
        for (int i = 0; i < out_elements; ++i) {
            float prob = (scale > 0.0f) ? (static_cast<int>(out_ptr[i]) - zero_point) * scale : out_ptr[i];
            scores.push_back({prob, i});
        }
    } else if (output_tensor->type == kTfLiteInt8) {
        int8_t* out_ptr = interpreter->typed_tensor<int8_t>(output_idx);
        float scale = output_tensor->params.scale;
        int zero_point = output_tensor->params.zero_point;
        for (int i = 0; i < out_elements; ++i) {
            float prob = (scale > 0.0f) ? (static_cast<int>(out_ptr[i]) - zero_point) * scale : out_ptr[i];
            scores.push_back({prob, i});
        }
    } else if (output_tensor->type == kTfLiteFloat32) {
        float* out_ptr = interpreter->typed_tensor<float>(output_idx);
        for (int i = 0; i < out_elements; ++i) {
            scores.push_back({out_ptr[i], i});
        }
    }

    std::sort(scores.begin(), scores.end(), [](const auto& a, const auto& b) {
        return a.first > b.first;
    });

    int top_k = std::min(5, static_cast<int>(scores.size()));
    for (int k = 0; k < top_k; ++k) {
        int cls_idx = scores[k].second;
        float score = scores[k].first;
        std::string label = (cls_idx < static_cast<int>(labels.size())) ? labels[cls_idx] : "Unknown";
        std::cout << "  #" << (k + 1) << " Class " << std::setw(4) << cls_idx
                  << " | Score: " << std::setw(8) << score
                  << " | Label: " << label << "\n";
    }

    std::cout << "\n[PASS] TensorFlow Lite C++ Inference Verification Complete.\n";
    std::cout << "========================================================\n";
    return 0;
}
