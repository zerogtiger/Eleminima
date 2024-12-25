#ifndef IMAGE_H
#define IMAGE_H

#include <cmath>
#include <complex>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <stdexcept>
#include <stdint.h>
#include <utility>
#include <vector>

#include "Adjustment.h"
#include "Color.h"
#include "Enums.h"
#include "Font.h"
#include "lib/schrift.h"

#define _USE_MATH_DEFINES

struct Image {
    uint8_t* data = NULL; // Bytes of image
    size_t size = 0;      // size of data
    int w;
    int h;
    int channels;

    Image(const char* filename);
    Image(int w, int h, int channels);
    Image(int w, int h, int channels, Color fill);
    Image(const Image& img);
    // Image(const Image& img, int channel);
    ~Image(); // destructor

    bool read(const char* filename);
    bool write(const char* filename);

    uint8_t get(uint32_t row, uint32_t col, uint32_t channel = 0);
    uint8_t get_or_default(int row, int col, uint32_t channel = 0, uint8_t fallback = 0);
    uint8_t get_offset(int row, int col, uint32_t offset_r, uint32_t offset_c, uint32_t channel = 0);
    uint8_t get_offset_or_default(int row, int col, uint32_t offset_r, uint32_t offset_c, uint32_t channel = 0,
                                  uint8_t fallback = 0);
    Color get_color(uint32_t row, uint32_t col);
    Color get_color_or_default(int row, int col, Color fallback = Color(0, 0, 0));

    bool set(uint32_t row, uint32_t col, uint32_t channel, uint8_t val);
    bool set_offset(int row, int col, uint32_t offset_r, uint32_t offset_c, uint32_t channel, uint8_t val);

    ImageType getFileType(const char* filename);

    Image& grayscale_avg();
    Image& grayscale_lum();

    Image& color_mask(float r, float g, float b);

    Image& encodemessage(const char* message);

    Image& decodemessage(char* buffer, size_t* messageLength);

    Image& diffmap(Image& img);
    Image& diffmap_scale(Image& img, uint8_t scl = 1);

    Image& std_convolve_clamp_to_zero(uint8_t channel, uint32_t ker_w, uint32_t ker_h, double ker[], uint32_t cr,
                                      uint32_t cc, bool normalize = false);
    Image& std_convolve_clamp_to_border(uint8_t channel, uint32_t ker_w, uint32_t ker_h, double ker[], uint32_t cr,
                                        uint32_t cc, bool normalize = false);
    Image& std_convolve_cyclic(uint8_t channel, uint32_t ker_w, uint32_t ker_h, double ker[], uint32_t cr, uint32_t cc,
                               bool normalize = false);

    // void std_convolve_clamp_to_zero_double(double* new_data, uint8_t channel, uint32_t ker_w, uint32_t ker_h,
    // double ker[], uint32_t cr, uint32_t cc);
    Image& flip_x();
    Image& flip_y();

    Image& overlay(const Image& src, int x, int y);

    Image& overlay_text(const char* txt, const Font& font, int x, int y, uint8_t r = 255, uint8_t g = 255,
                        uint8_t b = 255, uint8_t a = 255);

    Image& crop(uint16_t cx, uint16_t cy, uint16_t cw, uint16_t ch);
    static uint32_t rev(uint32_t n, uint32_t a); // n: 2^a
    static void bit_rev(uint32_t n, std::complex<double> a[], std::complex<double>* A);

    static void fft(uint32_t n, std::complex<double> x[], std::complex<double>* X);
    static void ifft(uint32_t n, std::complex<double> x[], std::complex<double>* X);
    static void dft_2D(uint32_t m, uint32_t n, std::complex<double> x[], std::complex<double>* X);
    static void idft_2D(uint32_t m, uint32_t n, std::complex<double> x[], std::complex<double>* X);

    static void pad_kernel(uint32_t ker_w, uint32_t ker_h, double ker[], uint32_t cr, uint32_t cc, uint32_t pw,
                           uint32_t ph, std::complex<double>* pad_ker);
    static inline void pointwise_product(uint64_t l, std::complex<double> a[], std::complex<double> b[],
                                         std::complex<double>* p);

    std::complex<double>* fd_convolve_clamp_to_zero_raw(uint8_t channel, uint32_t ker_w, uint32_t ker_h, double ker[],
                                                        uint32_t cr, uint32_t cc);
    Image& fd_convolve_clamp_to_zero(uint8_t channel, uint32_t ker_w, uint32_t ker_h, double ker[], uint32_t cr,
                                     uint32_t cc, bool normalize = false);
    std::complex<double>* fd_convolve_clamp_to_border_raw(uint8_t channel, uint32_t ker_w, uint32_t ker_h, double ker[],
                                                          uint32_t cr, uint32_t cc);
    Image& fd_convolve_clamp_to_border(uint8_t channel, uint32_t ker_w, uint32_t ker_h, double ker[], uint32_t cr,
                                       uint32_t cc, bool normalize = false);
    std::complex<double>* fd_convolve_cyclic_raw(uint8_t channel, uint32_t ker_w, uint32_t ker_h, double ker[],
                                                 uint32_t cr, uint32_t cc);
    Image& fd_convolve_cyclic(uint8_t channel, uint32_t ker_w, uint32_t ker_h, double ker[], uint32_t cr, uint32_t cc,
                              bool normalize = false);

    Image& convolve_linear(uint8_t channel, uint32_t ker_w, uint32_t ker_h, double ker[], uint32_t cr, uint32_t cc,
                           bool normalize = false);
    Image& convolve_clamp_to_border(uint8_t channel, uint32_t ker_w, uint32_t ker_h, double ker[], uint32_t cr,
                                    uint32_t cc, bool normalize = false);
    Image& convolve_cyclic(uint8_t channel, uint32_t ker_w, uint32_t ker_h, double ker[], uint32_t cr, uint32_t cc,
                           bool normalize = false);

    Image& brightness(uint8_t channel, double brightness_delta);
    Image& contrast(uint8_t channel, double contrast_delta);
    // implemented, but unnecessary
    Image& saturation(int channel, double saturation_delta);
    Image& exposure(double exposure);

    Image& shade_h();
    Image& shade_v();
    Image& shade();
    Image& edge(bool gradient = false, double detail_threshold = 0.09);

    Image& f_scale(uint32_t new_w, uint32_t new_h, bool linked = false, TwoDimInterp method = TwoDimInterp::Nearest);
    Image& translate(int x, int y, Color fill = Color(0, 0, 0, 255));
    Image& rotate(double origin_x, double origin_y, double angle, TwoDimInterp method = TwoDimInterp::Bilinear,
                  Color fill = Color(0, 0, 0));

    Image& invert_color(uint8_t channel);
    Image& gamma(uint8_t channel, double gamma_delta);

    Image& color_reduce(ColorDepth depth = ColorDepth::Bit_3, bool error_diffusion = true);

    Image& color_ramp(std::vector<std::pair<double, Color>> points, OneDimInterp method = OneDimInterp::Linear);
    Image& preview_color_ramp(std::vector<std::pair<double, Color>> points,
                              OneDimInterp method = OneDimInterp::Linear) const;

    std::vector<Image*> separate_channels();
    Image& combine_channels(std::vector<Image*> imgs, bool resize_to_fit = false,
                            TwoDimInterp method = TwoDimInterp::Bilinear);
    Image& set_alpha(Image& alph, bool resize_to_fit = false, TwoDimInterp method = TwoDimInterp::Bilinear);

    Image& color_balance(Color lift, Color gamma, Color gain);

    Image& histogram(bool inc_lum = true, int channel = -1, Color fill = Color(125, 125, 125));
    Image& histogram_lum(Color fill = Color(125, 125, 125));
    Image& histogram_avg(Color fill = Color(125, 125, 125));

    Image& HSV(double hue_delta, double saturation_delta, double value_delta);

    Image& false_color(bool overwrite = false);

    Image& tone_correct(uint8_t midtones_start, uint8_t midtones_end, Adjustment shadow, Adjustment midtone,
                        Adjustment highlight);


    Image& RGB_curves(OneDimInterp method = OneDimInterp::Bezier,
                      std::vector<std::pair<double, double>> control_c = {{0, 0}, {0, 0}, {1, 1}, {1, 1}},
                      std::vector<std::pair<double, double>> control_r = {{0, 0}, {0, 0}, {1, 1}, {1, 1}},
                      std::vector<std::pair<double, double>> control_g = {{0, 0}, {0, 0}, {1, 1}, {1, 1}},
                      std::vector<std::pair<double, double>> control_b = {{0, 0}, {0, 0}, {1, 1}, {1, 1}});

    Image& preview_RGB_curves(OneDimInterp method = OneDimInterp::Bezier,
                              std::vector<std::pair<double, double>> control_c = {{0, 0}, {0, 0}, {1, 1}, {1, 1}},
                              std::vector<std::pair<double, double>> control_r = {{0, 0}, {0, 0}, {1, 1}, {1, 1}},
                              std::vector<std::pair<double, double>> control_g = {{0, 0}, {0, 0}, {1, 1}, {1, 1}},
                              std::vector<std::pair<double, double>> control_b = {{0, 0}, {0, 0}, {1, 1}, {1, 1}});

    Image& hue_correct(std::vector<std::pair<double, double>> control_h = {{179, 0}, {180, 0}, {181, 0}},
                       std::vector<std::pair<double, double>> control_s = {{179, 0}, {180, 0}, {181, 0}},
                       std::vector<std::pair<double, double>> control_v = {{179, 0}, {180, 0}, {181, 0}});

    Image& preview_hue_correct(std::vector<std::pair<double, double>> control_h = {{179, 0}, {180, 0}, {181, 0}},
                               std::vector<std::pair<double, double>> control_s = {{179, 0}, {180, 0}, {181, 0}},
                               std::vector<std::pair<double, double>> control_v = {{179, 0}, {180, 0}, {181, 0}});

    Image& blur(Blur method = Blur::Gaussian, int radius_x = 5, int radius_y = 5);

    Image& alpha_overlay(Image* fac, int fac_x, int fac_y, Image* other, int other_x, int other_y);
    Image& alpha_overlay(Color color, Image* other, int other_x, int other_y);
    Image& alpha_overlay(Color color, Color other);
    Image& alpha_overlay(Image* fac, int fac_x, int fac_y, Color other);

    Image& vignette(double fact_x = 0, double fact_y = 0);

    Image& color_matrix(std::vector<std::vector<double>> matrix);

    Image& rect_mask(double start_x, double start_y, double end_x, double end_y, Color fill = Color(255, 255, 255));
    Image& ellipse_mask(double center_x, double center_y, double radius_x, double radius_y,
                        Color fill = Color(255, 255, 255));

    Image& white_noise(double min = 0.0, double max = 255.0, bool color = true, int seed = 0);
    Image& perlin_noise(double min = 0.0, double max = 255.0, int grid_size = 400, double freq = 1, double amp = 1,
                        int seed = 0);
};
#endif
