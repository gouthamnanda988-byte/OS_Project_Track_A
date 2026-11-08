#ifndef IMAGE_LOADER_H
#define IMAGE_LOADER_H

#include <string>
#include <vector>
#include <cstdint>

struct ImageRGB {
    int width = 0;
    int height = 0;
    int channels = 3; // Always RGB
    std::vector<uint8_t> data; // Size: width * height * 3

    bool is_valid() const {
        return width > 0 && height > 0 && data.size() == static_cast<size_t>(width * height * 3);
    }
};

class ImageLoader {
public:
    // Load a PPM image (supports both P6 binary and P3 ASCII)
    static bool load_ppm(const std::string& filepath, ImageRGB& out_image, std::string& error_msg);

    // Save an image to PPM P6 binary format (useful for testing and debug)
    static bool save_ppm_p6(const std::string& filepath, const ImageRGB& image, std::string& error_msg);

    // Create a synthetic test pattern image (gradient / color bars)
    static ImageRGB create_test_pattern(int width, int height);
};

#endif // IMAGE_LOADER_H
