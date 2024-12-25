#include "../src/Image.h"

extern "C" {
    typedef struct ImageWrapper {
        void* instance;
    } ImageWrapper;

    // Constructor Wrappers
    ImageWrapper* image_create_from_file(const char* filename) {
        return new ImageWrapper{ new Image(filename) };
    }

    // ImageWrapper* image_create(int w, int h, int channels) {
    //     return new ImageWrapper{ new Image(w, h, channels) };
    // }

    // Destructor Wrapper
    void image_destroy(ImageWrapper* img) {
        delete static_cast<Image*>(img->instance);
        delete img;
    }

    // Method Wrappers
    bool image_read(ImageWrapper* img, const char* filename) {
        return static_cast<Image*>(img->instance)->read(filename);
    }

    bool image_write(ImageWrapper* img, const char* filename) {
        return static_cast<Image*>(img->instance)->write(filename);
    }

    // uint8_t image_get_pixel(ImageWrapper* img, uint32_t row, uint32_t col, uint32_t channel) {
    //     return static_cast<Image*>(img->instance)->get(row, col, channel);
    // }
    //
    // bool image_set_pixel(ImageWrapper* img, uint32_t row, uint32_t col, uint32_t channel, uint8_t value) {
    //     return static_cast<Image*>(img->instance)->set(row, col, channel, value);
    // }

    // Grayscale example
    void image_grayscale_avg(ImageWrapper* img) {
        static_cast<Image*>(img->instance)->grayscale_avg();
    }

    void image_grayscale_lum(ImageWrapper* img) {
        static_cast<Image*>(img->instance)->grayscale_lum();
    }
}

