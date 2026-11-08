#include "image_loader.h"

#include <fstream>
#include <sstream>
#include <iostream>
#include <cctype>

// Helper: skip whitespace and PPM comments (starting with '#')
static void skip_comments_and_whitespace(std::istream& in) {
    while (in.good()) {
        in >> std::ws;
        if (in.peek() == '#') {
            std::string comment;
            std::getline(in, comment);
        } else {
            break;
        }
    }
}

bool ImageLoader::load_ppm(const std::string& filepath, ImageRGB& out_image, std::string& error_msg) {
    std::ifstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        error_msg = "Cannot open file: " + filepath;
        return false;
    }

    // 1. Read Magic Number
    std::string magic;
    skip_comments_and_whitespace(file);
    if (!(file >> magic)) {
        error_msg = "Failed to read PPM magic number from " + filepath;
        return false;
    }

    if (magic != "P6" && magic != "P3") {
        error_msg = "Unsupported PPM format '" + magic + "'. Only P6 (binary) and P3 (ASCII) are supported.";
        return false;
    }

    // 2. Read Dimensions
    skip_comments_and_whitespace(file);
    int width = 0;
    int height = 0;
    int maxval = 0;

    if (!(file >> width)) {
        error_msg = "Failed to read image width.";
        return false;
    }

    skip_comments_and_whitespace(file);
    if (!(file >> height)) {
        error_msg = "Failed to read image height.";
        return false;
    }

    skip_comments_and_whitespace(file);
    if (!(file >> maxval)) {
        error_msg = "Failed to read image maxval.";
        return false;
    }

    if (width <= 0 || height <= 0 || width > 8192 || height > 8192) {
        error_msg = "Invalid image dimensions: " + std::to_string(width) + "x" + std::to_string(height);
        return false;
    }

    if (maxval <= 0 || maxval > 65535) {
        error_msg = "Invalid maxval: " + std::to_string(maxval) + " (expected 1..65535)";
        return false;
    }

    // Skip single trailing whitespace/newline byte after maxval before pixel data
    file.get();

    out_image.width = width;
    out_image.height = height;
    out_image.channels = 3;
    size_t total_pixels = static_cast<size_t>(width) * static_cast<size_t>(height);
    size_t total_bytes = total_pixels * 3;
    out_image.data.resize(total_bytes);

    // 3. Read Pixel Data
    if (magic == "P6") {
        if (maxval <= 255) {
            file.read(reinterpret_cast<char*>(out_image.data.data()), total_bytes);
            std::streamsize bytes_read = file.gcount();
            if (bytes_read != static_cast<std::streamsize>(total_bytes)) {
                error_msg = "Corrupted image data: expected " + std::to_string(total_bytes) +
                            " bytes, but read only " + std::to_string(bytes_read);
                return false;
            }
        } else {
            // 16-bit binary PPM -> scale down to 8-bit
            for (size_t i = 0; i < total_bytes; ++i) {
                uint8_t high = 0;
                uint8_t low = 0;
                if (!file.get(reinterpret_cast<char&>(high)) || !file.get(reinterpret_cast<char&>(low))) {
                    error_msg = "Premature EOF in 16-bit P6 PPM data.";
                    return false;
                }
                uint16_t val = (static_cast<uint16_t>(high) << 8) | low;
                out_image.data[i] = static_cast<uint8_t>((val * 255) / maxval);
            }
        }
    } else { // P3 ASCII
        for (size_t i = 0; i < total_bytes; ++i) {
            skip_comments_and_whitespace(file);
            int val = 0;
            if (!(file >> val)) {
                error_msg = "Corrupted P3 ASCII data: failed at byte index " + std::to_string(i);
                return false;
            }
            if (val < 0) val = 0;
            if (maxval != 255) {
                val = (val * 255) / maxval;
            }
            if (val > 255) val = 255;
            out_image.data[i] = static_cast<uint8_t>(val);
        }
    }

    return true;
}

bool ImageLoader::save_ppm_p6(const std::string& filepath, const ImageRGB& image, std::string& error_msg) {
    if (!image.is_valid()) {
        error_msg = "Invalid image buffer cannot be saved.";
        return false;
    }

    std::ofstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        error_msg = "Cannot open file for writing: " + filepath;
        return false;
    }

    file << "P6\n"
         << "# Edge ML Linux OS (Track A) Test Image\n"
         << image.width << " " << image.height << "\n"
         << "255\n";

    file.write(reinterpret_cast<const char*>(image.data.data()), image.data.size());
    if (!file.good()) {
        error_msg = "Error writing image data to file: " + filepath;
        return false;
    }

    return true;
}

ImageRGB ImageLoader::create_test_pattern(int width, int height) {
    ImageRGB img;
    img.width = width;
    img.height = height;
    img.channels = 3;
    img.data.resize(width * height * 3);

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            size_t idx = (y * width + x) * 3;
            // Generate deterministic smooth color pattern
            img.data[idx + 0] = static_cast<uint8_t>((x * 255) / width);         // Red gradient horizontal
            img.data[idx + 1] = static_cast<uint8_t>((y * 255) / height);        // Green gradient vertical
            img.data[idx + 2] = static_cast<uint8_t>(((x + y) * 128) / (width + height)); // Blue diagonal
        }
    }
    return img;
}
