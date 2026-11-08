#include <iostream>
#include <cassert>
#include <cmath>
#include <fstream>
#include "../src/image_loader.h"
#include "../src/image_preprocessor.h"

int main() {
    std::cout << ">>> Running Host-side Image Pipeline Unit Tests...\n";

    // 1. Test Synthetic Pattern Generation
    ImageRGB pattern = ImageLoader::create_test_pattern(300, 200);
    assert(pattern.width == 300);
    assert(pattern.height == 200);
    assert(pattern.is_valid());
    std::cout << "[PASS] Test pattern creation: 300x200 valid.\n";

    // 2. Test Save & Load P6 Binary PPM
    std::string err;
    std::string test_p6_file = "/tmp/test_image_p6.ppm";
    bool save_ok = ImageLoader::save_ppm_p6(test_p6_file, pattern, err);
    assert(save_ok);

    ImageRGB loaded_p6;
    bool load_ok = ImageLoader::load_ppm(test_p6_file, loaded_p6, err);
    assert(load_ok);
    assert(loaded_p6.width == 300);
    assert(loaded_p6.height == 200);
    assert(loaded_p6.data == pattern.data);
    std::cout << "[PASS] P6 Binary PPM save & load verified (exact byte match).\n";

    // 3. Test Load P3 ASCII PPM
    std::string test_p3_file = "/tmp/test_image_p3.ppm";
    {
        std::ofstream p3(test_p3_file);
        p3 << "P3\n# Sample ASCII comment\n2 2\n# Another comment\n255\n";
        p3 << "255 0 0   0 255 0\n";
        p3 << "0 0 255   255 255 255\n";
    }

    ImageRGB loaded_p3;
    bool load_p3_ok = ImageLoader::load_ppm(test_p3_file, loaded_p3, err);
    assert(load_p3_ok);
    assert(loaded_p3.width == 2);
    assert(loaded_p3.height == 2);
    // Pixel 0,0: Red (255, 0, 0)
    assert(loaded_p3.data[0] == 255 && loaded_p3.data[1] == 0 && loaded_p3.data[2] == 0);
    // Pixel 0,1: Green (0, 255, 0)
    assert(loaded_p3.data[3] == 0 && loaded_p3.data[4] == 255 && loaded_p3.data[5] == 0);
    std::cout << "[PASS] P3 ASCII PPM load with comments verified.\n";

    // 4. Test Bilinear Resize
    ImageRGB resized = ImagePreprocessor::resize_bilinear(pattern, 224, 224);
    assert(resized.width == 224);
    assert(resized.height == 224);
    assert(resized.is_valid());
    std::cout << "[PASS] Bilinear resize from 300x200 -> 224x224 verified.\n";

    // 5. Test Corrupt File Handling
    std::string corrupt_file = "/tmp/test_corrupt.ppm";
    {
        std::ofstream cfile(corrupt_file);
        cfile << "P6\n10 10\n255\n"; // Missing pixel bytes
    }
    ImageRGB corrupt_img;
    bool corrupt_res = ImageLoader::load_ppm(corrupt_file, corrupt_img, err);
    assert(!corrupt_res);
    assert(!err.empty());
    std::cout << "[PASS] Corrupted image safely rejected with error: " << err << "\n";

    std::cout << ">>> All Host-side Image Pipeline Unit Tests PASSED!\n";
    return 0;
}
