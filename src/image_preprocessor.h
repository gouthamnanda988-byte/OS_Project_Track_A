#ifndef IMAGE_PREPROCESSOR_H
#define IMAGE_PREPROCESSOR_H

#include "image_loader.h"
#include <string>
#include <cstdint>

// Forward declaration of TfLiteTensor
struct TfLiteTensor;

class ImagePreprocessor {
public:
    // Bilinear interpolation resize from src to (target_w, target_h)
    static ImageRGB resize_bilinear(const ImageRGB& src, int target_w, int target_h);

    // Populate a TensorFlow Lite input tensor from an RGB image buffer with appropriate quantization
    static bool populate_input_tensor(const ImageRGB& image, TfLiteTensor* tensor, std::string& error_msg);
};

#endif // IMAGE_PREPROCESSOR_H
