#include "image_preprocessor.h"

#include <cmath>
#include <algorithm>
#include <iostream>

#include "tensorflow/lite/c/common.h"

ImageRGB ImagePreprocessor::resize_bilinear(const ImageRGB& src, int target_w, int target_h) {
    ImageRGB dst;
    dst.width = target_w;
    dst.height = target_h;
    dst.channels = 3;
    dst.data.resize(target_w * target_h * 3);

    if (src.width == target_w && src.height == target_h) {
        dst.data = src.data;
        return dst;
    }

    float x_ratio = (src.width > 1) ? static_cast<float>(src.width - 1) / (target_w - 1) : 0.0f;
    float y_ratio = (src.height > 1) ? static_cast<float>(src.height - 1) / (target_h - 1) : 0.0f;

    for (int y = 0; y < target_h; ++y) {
        float src_y = y * y_ratio;
        int y_low = static_cast<int>(src_y);
        int y_high = std::min(y_low + 1, src.height - 1);
        float y_weight = src_y - y_low;

        for (int x = 0; x < target_w; ++x) {
            float src_x = x * x_ratio;
            int x_low = static_cast<int>(src_x);
            int x_high = std::min(x_low + 1, src.width - 1);
            float x_weight = src_x - x_low;

            size_t dst_idx = (y * target_w + x) * 3;

            size_t c00 = (y_low * src.width + x_low) * 3;
            size_t c10 = (y_low * src.width + x_high) * 3;
            size_t c01 = (y_high * src.width + x_low) * 3;
            size_t c11 = (y_high * src.width + x_high) * 3;

            for (int c = 0; c < 3; ++c) {
                float a = src.data[c00 + c];
                float b = src.data[c10 + c];
                float d = src.data[c01 + c];
                float e = src.data[c11 + c];

                float top = a * (1.0f - x_weight) + b * x_weight;
                float bottom = d * (1.0f - x_weight) + e * x_weight;
                float pixel = top * (1.0f - y_weight) + bottom * y_weight;

                int val = static_cast<int>(std::round(pixel));
                dst.data[dst_idx + c] = static_cast<uint8_t>(std::clamp(val, 0, 255));
            }
        }
    }

    return dst;
}

bool ImagePreprocessor::populate_input_tensor(const ImageRGB& image, TfLiteTensor* tensor, std::string& error_msg) {
    if (!tensor) {
        error_msg = "Null input tensor pointer.";
        return false;
    }

    if (!image.is_valid()) {
        error_msg = "Invalid input image buffer.";
        return false;
    }

    // Verify tensor dimensions: expect [batch, height, width, channels]
    if (tensor->dims->size != 4) {
        error_msg = "Expected 4D input tensor [B, H, W, C], but got " + std::to_string(tensor->dims->size) + "D.";
        return false;
    }

    int expected_h = tensor->dims->data[1];
    int expected_w = tensor->dims->data[2];
    int expected_c = tensor->dims->data[3];

    if (image.width != expected_w || image.height != expected_h || image.channels != expected_c) {
        error_msg = "Image dimensions (" + std::to_string(image.width) + "x" + std::to_string(image.height) +
                    "x" + std::to_string(image.channels) + ") do not match tensor expected dimensions (" +
                    std::to_string(expected_w) + "x" + std::to_string(expected_h) + "x" + std::to_string(expected_c) + ").";
        return false;
    }

    size_t num_elements = static_cast<size_t>(expected_h * expected_w * expected_c);
    float scale = tensor->params.scale;
    int zero_point = tensor->params.zero_point;

    switch (tensor->type) {
        case kTfLiteUInt8: {
            uint8_t* dst = tensor->data.uint8;
            if (scale > 0.0f) {
                // If model scale is ~1/128 and zero_point is 128, standard MobileNet normalization:
                // float norm = (raw / 127.5f) - 1.0f;
                // uint8_t quantized = round(norm / scale) + zero_point;
                // For scale=0.0078125 and zero_point=128:
                // (raw/127.5 - 1.0)*128 + 128 == raw (identity with slight rounding)
                for (size_t i = 0; i < num_elements; ++i) {
                    float norm_f = (static_cast<float>(image.data[i]) / 127.5f) - 1.0f;
                    int q = static_cast<int>(std::round(norm_f / scale)) + zero_point;
                    dst[i] = static_cast<uint8_t>(std::clamp(q, 0, 255));
                }
            } else {
                std::copy(image.data.begin(), image.data.end(), dst);
            }
            break;
        }

        case kTfLiteInt8: {
            int8_t* dst = tensor->data.int8;
            for (size_t i = 0; i < num_elements; ++i) {
                float norm_f = (static_cast<float>(image.data[i]) / 127.5f) - 1.0f;
                int q = (scale > 0.0f) ? static_cast<int>(std::round(norm_f / scale)) + zero_point
                                       : static_cast<int>(std::round(norm_f * 128.0f));
                dst[i] = static_cast<int8_t>(std::clamp(q, -128, 127));
            }
            break;
        }

        case kTfLiteFloat32: {
            float* dst = tensor->data.f;
            for (size_t i = 0; i < num_elements; ++i) {
                dst[i] = (static_cast<float>(image.data[i]) / 127.5f) - 1.0f;
            }
            break;
        }

        default:
            error_msg = "Unsupported input tensor type: " + std::string(TfLiteTypeGetName(tensor->type));
            return false;
    }

    return true;
}
