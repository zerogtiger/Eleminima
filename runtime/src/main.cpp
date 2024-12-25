#include "Adjustment.h"
#include "Color.h"
#include "Enums.h"
#include "Image.h"
#include "Interpolation.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <i386/endian.h>
#include <iostream>
#include <iterator>
#include <pthread.h>
#include <string>
#include <strstream>
#include <vector>

void make_random_complex_arr(uint32_t len, std::complex<double>* z) {
    for (uint32_t i = 0; i < len; i++) {
        z[i] = std::complex<double>(rand() % 100, rand() % 100);
    }
}

void print_complex_arr(uint32_t len, std::complex<double> z[]) {
    for (uint32_t i = 0; i < len; i++) {
        printf("%f + %fi, \n", z[i].real(), z[i].imag());
    }
    printf("\n");
}

void matrix_scalar(double* arr, int size, double scalar) {
    for (int i = 0; i < size; i++) {
        arr[i] /= scalar;
    }
}

void test1() {

    Image test("images/sobel_test.jpg");

    // grayscale
    test.grayscale_avg();
    int test_size = test.w * test.h;

    Image gray_test(test.w, test.h, 1);
    for (uint64_t k = 0; k < test_size; k++) {
        gray_test.data[k] = test.data[k * test.channels];
    }
    gray_test.write("images/test_gray.png");

    double gaussian_blur[] = {1 / 16.0, 2 / 16.0, 1 / 16.0, 2 / 16.0, 4 / 16.0, 2 / 16.0, 1 / 16.0, 2 / 16.0, 1 / 16.0};

    gray_test.convolve_linear(0, 3, 3, gaussian_blur, 1, 1);
    gray_test.write("images/test_blur.png");
    Image blur_test(gray_test);

    Image blur_test_x(blur_test);
    Image blur_test_y(blur_test);

    double scharr_x[] = {47, 0, -47, 162, 0, -162, 47, 0, -47};
    double scharr_y[] = {47, 162, 47, 0, 0, 0, -47, -162, -47};
    blur_test_x.fd_convolve_clamp_to_border(0, 3, 3, scharr_x, 1, 1, false);
    blur_test_y.fd_convolve_clamp_to_zero(0, 3, 3, scharr_y, 1, 1, true);
    blur_test_x.write("images/blur_test_x.png");
    blur_test_y.write("images/blur_test_y.png");

    // make test image
    double mxx = -INFINITY, mxy = -INFINITY, mnx = INFINITY, mny = INFINITY;

    // gets rid of insignificant edges
    double threshold = 0.09;
    double* g = new double[test_size];     // percent
    double* theta = new double[test_size]; // direction
    double x, y;
    for (uint64_t k = 0; k < test_size; k++) {
        x = blur_test_x.data[k * blur_test_x.channels];
        y = blur_test_y.data[k * blur_test_y.channels];
        g[k] = sqrt(x * x / 255 / 255 + y * y / 255 / 255);
        theta[k] = atan2(y, x);
    }

    // makes image hsl -> rgb
    double mx = -INFINITY, mn = INFINITY;

    for (uint64_t k = 0; k < test_size; k++) {
        mx = fmax(mx, g[k]);
        mn = fmin(mn, g[k]);
    }
    Image G(test.w, test.h, 1);
    Image GT(test.w, test.h, 3);

    double h, s, l;
    double v;
    for (uint64_t k = 0; k < test_size; k++) {
        h = theta[k] * 180.0 / M_PI + 180;

        if (mx == mn) {
            v = 0;
        }
        else {
            v = (g[k] - mn) / (mx - mn) > threshold ? (g[k] - mn) / (mx - mn) : 0;
        }
        s = l = v;

        Color c(0, 0, 0);
        c.hsv_to_rgb(h, v, v);

        GT.data[k * 3] = c.r;
        GT.data[k * 3 + 1] = c.g;
        GT.data[k * 3 + 2] = c.b;
        G.data[k] = (uint8_t) (255 * v);
    }

    G.write("images/G.png");
    GT.write("images/GT.png");
    delete[] g;
    delete[] theta;

    //
    //     const uint32_t len = 16;
    //     std::complex<double>* a = new std::complex<double>[len];
    //     make_random_complex_arr(len, a);
    //     print_complex_arr(len, a);
    //
    //     std::complex<double>* A = new std::complex<double>[len];
    //     std::complex<double>* a_recovered = new std::complex<double>[len];
    //
    //     auto fft_2D_start = std::chrono::system_clock::now();
    //     Image::dft_2D(4, 4, a, A);
    //     auto fft_2D_end = std::chrono::system_clock::now();
    //
    //     print_complex_arr(len, A);
    //
    //     auto ifft_2D_start = std::chrono::system_clock::now();
    //     Image::idft_2D(4, 4, A, a_recovered);
    //     auto ifft_2D_end = std::chrono::system_clock::now();
    //     print_complex_arr(len, a_recovered);
    //
    //     printf("2D fft took %lldns\n", std::chrono::duration_cast<std::chrono::nanoseconds>(fft_2D_end -
    //     fft_2D_start).count()); printf("2D ifft took %lldns\n",
    //     std::chrono::duration_cast<std::chrono::nanoseconds>(ifft_2D_end - ifft_2D_start).count());
    //
    //     delete[] a;
    //     delete[] A;
    //     delete[] a_recovered;

    // Image test("test1.jpg");

    // test.crop(100, 100, 3000, 4000);
    // test.write("cropped.png");

    // Font iosevka("Iosevka Term Nerd Font Complete.ttf", 100);
    // test.overlay_text("TEST", iosevka, 300, 300, 255, 0, 0, 100); // seg fault when 100, 100 for position
    // test.write("overlay_text.png");
    //
    // Image overlay("clear_bg.png");
    // test.overlay(overlay, 400, 500);
    // test.write("overlayed.png");

    // test.flip_y();
    // test.flip_x();
    // test.write("flipped_x.png");

    // embossing
    // double ridge[] = {0, -1, 0, -1, 4, -1, 0, -1, 0};
    // double edge[] = {-1, -1, -1, -1, 8, -1, -1, -1, -1};
    // double sharpen[] = {0, -1, 0, -1, 5, -1, 0, -1, 0};
    // double emboss[] = {-2 / 3.0, -1 / 3.0, 0, -1 / 3.0, 1 / 3.0, 1 / 3.0, 0, 1 / 3.0, 2 / 3.0};
    // double g_blur[] = {1, 4, 6, 4, 1, 4, 16, 24, 16, 4, 6, 24, 36, 24, 6, 4, 16, 24, 16, 4, 1, 4, 6, 4, 1};
    // double unsharp[] = {1, 4, 6, 4, 1, 4, 16, 24, 16, 4, 6, 24, -476, 24, 6, 4, 16, 24, 16, 4, 1, 4, 6, 4, 1};
    // matrix_scalar(g_blur, 25, 256);
    // matrix_scalar(unsharp, 25, -256.0);
    // double emboss[] = {-2 / 3.0, -1 / 3.0, 0, -1 / 3.0, 1 / 3.0, 1 / 3.0, 0, 1 / 3.0, 2 / 3.0};
    // double gaussian_blur[] = {
    //     1/16.0, 2/16.0, 1/16.0,
    //     2/16.0, 4/16.0, 2/16.0,
    //     1/16.0, 2/16.0, 1/16.0
    // };

    // Image t1("colorful2.jpg");
    // Image t2 = t1;
    // Image t0 = t1;
    // t1.std_convolve_clamp_to_border(0, 5, 5, unsharp, 2, 2);
    // t1.std_convolve_clamp_to_border(1, 5, 5, unsharp, 2, 2);
    // t1.std_convolve_clamp_to_border(2, 5, 5, unsharp, 2, 2);
    // t1.std_convolve_clamp_to_border(0, 3, 3, gaussian_blur, 1, 1);
    // t1.std_convolve_clamp_to_border(1, 3, 3, gaussian_blur, 1, 1);
    // t1.std_convolve_clamp_to_border(2, 3, 3, gaussian_blur, 1, 1);
    // t1.std_convolve_clamp_to_border(0, 5, 5, g_blur, 2, 2);
    // t1.std_convolve_clamp_to_border(1, 5, 5, g_blur, 2, 2);
    // t1.std_convolve_clamp_to_border(2, 5, 5, g_blur, 2, 2);
    // t1.fd_convolve_clamp_to_border(0, 5, 5, g_blur, 2, 2);
    // t1.fd_convolve_clamp_to_border(1, 5, 5, g_blur, 2, 2);
    // t1.fd_convolve_clamp_to_border(2, 5, 5, g_blur, 2, 2);
    // t1.write("dft_g_blur_5x5.png");
    // t2.std_convolve_clamp_to_zero(0, 3, 3, gaussian_blur, 1, 1);
    // t2.std_convolve_clamp_to_zero(1, 3, 3, gaussian_blur, 1, 1);
    // t2.std_convolve_clamp_to_zero(2, 3, 3, gaussian_blur, 1, 1);
    //
    // t1.diffmap_scale(t0);
    // t1.write("con.png");

    // Image orig("test1.jpg");
    //
    // test1.diffmap_scale(orig);
    // test1.write("cov_diff.png");

    // Image test("test.jpg");
    // Image test2("test1.jpg");
    // Image out2("out2.png");
    // Image sec("secret.png");
    // test1.diffmap_scale(sec);
    // test1.write("secret_dif_s.png");

    // test.encodemessage("Secret message");
    //
    // test.write("secret.png");
    //
    // Image secret("secret.png");
    //
    // char buffer[256] = {0};
    // size_t len = 0;
    // secret.decodemessage(buffer, &len);
    //
    // printf("Message: %s (%zu)\n", buffer, len);

    // test.color_mask(1, 0.8, 0.9);
    // test.write("out2.png");

    // test.grayscale_lum();
    // test.write("out1.png");

    // test.write("new.png");
    // Image copy = test;
    // copy.write("out.png");
    // Image blank(100, 100, 3);
    // for (int i = 0; i < blank.w * blank.channels; ++i) {
    //     blank.data[i] = 255;
    // }
    // blank.write("blank.jpg");
}

void demo() {
    Image demo("demo/original/demo.jpeg");
    Image demo2("demo/original/demo_2.jpeg");
    Image demo3("demo/original/demo_3.png");

    Image greyscale_avg(demo);
    greyscale_avg.grayscale_avg();
    greyscale_avg.write("demo/greyscale_avg.png");

    Image greyscale_lum(demo);
    greyscale_lum.grayscale_lum();
    greyscale_lum.write("demo/greyscale_lum.png");

    Image color_mask(demo);
    color_mask.color_mask(0.6, 1, 0.2);
    color_mask.write("demo/color_mask.png");

    Image diffmap(greyscale_lum);
    diffmap.diffmap(greyscale_avg);
    diffmap.write("demo/diffmap.png");

    Image diffmap_scale(greyscale_lum);
    diffmap.diffmap_scale(greyscale_avg);
    diffmap_scale.write("demo/diffmap_scale.png");

    Image convolve(demo);
    double scharr_x[] = {47, 0, -47, 162, 0, -162, 47, 0, -47};
    convolve.convolve_clamp_to_border(0, 3, 3, scharr_x, 1, 1, true);
    convolve.convolve_clamp_to_border(1, 3, 3, scharr_x, 1, 1, true);
    convolve.convolve_clamp_to_border(2, 3, 3, scharr_x, 1, 1, true);
    convolve.write("demo/convolve.png");

    Image flip_x(demo);
    Image flip_y(demo);
    flip_x.flip_x();
    flip_y.flip_y();
    flip_x.write("demo/flip_x.png");
    flip_y.write("demo/flip_y.png");

    Image overlay(demo);
    overlay.overlay(demo3, 300, 10);
    overlay.write("demo/overlay.png");

    Image overlay_text(demo);
    Font iosevka("demo/original/Iosevka Term Nerd Font Complete.ttf", 160);
    overlay_text.overlay_text("Some Text", iosevka, 100, 300, 255, 0, 0, 255);
    overlay_text.write("demo/overlay_text.png");

    Image crop(demo);
    crop.crop(300, 300, 400, 400);
    crop.write("demo/crop.png");

    Image brightness(demo);
    brightness.brightness(0, 80);
    brightness.brightness(1, 80);
    brightness.brightness(2, 80);
    brightness.write("demo/brightness.png");

    Image contrast(demo);
    contrast.contrast(0, 80);
    contrast.contrast(1, 80);
    contrast.contrast(2, 80);
    contrast.write("demo/contrast.png");

    Image saturation(demo);
    saturation.saturation(0, 0.5);
    saturation.saturation(1, 0.5);
    saturation.saturation(2, 0.5);
    saturation.write("demo/saturation.png");

    Image exposure(demo);
    exposure.exposure(1);
    exposure.write("demo/exposure.png");

    Image shade_h(demo);
    Image shade_v(demo);
    Image shade(demo);
    shade_h.shade_h();
    shade_v.shade_v();
    shade.shade();
    shade_h.write("demo/shade_h.png");
    shade_v.write("demo/shade_v.png");
    shade.write("demo/shade.png");

    Image edge(demo);
    edge.edge(true);
    edge.write("demo/edge.png");

    Image f_scale(demo);
    f_scale.f_scale(400, 400, false, TwoDimInterp::Bilinear);
    f_scale.write("demo/f_scale.png");

    Image translate(demo);
    translate.translate(300, 300);
    translate.write("demo/translate.png");

    Image rotate(demo);
    rotate.rotate(300, 500, 30);
    rotate.write("demo/rotate.png");

    Image invert_color(demo);
    invert_color.invert_color(0);
    invert_color.invert_color(1);
    invert_color.invert_color(2);
    invert_color.write("demo/invert_color.png");

    Image gamma(demo);
    gamma.gamma(0, 2.2);
    gamma.gamma(1, 2.2);
    gamma.gamma(2, 2.2);
    gamma.write("demo/gamma.png");

    Image color_reduce(demo);
    color_reduce.color_reduce(ColorDepth::Bit_3, true);
    color_reduce.write("demo/color_reduce.png");

    Image color_ramp(demo);
    std::vector<std::pair<double, Color>> color_ramp_points{{0.0, Color(0.0, 100.0, 100.0)},
                                                            {0.3, Color(100.0, 0.0, 100.0)},
                                                            {0.6, Color(200.0, 100.0, 10.0)},
                                                            {0.9, Color(0, 255, 255)},
                                                            {1.0, Color(100, 200, 100)}};
    Image preview_color_ramp = color_ramp.preview_color_ramp(color_ramp_points);
    preview_color_ramp.write("demo/preview_color_ramp.png");
    color_ramp.color_ramp(color_ramp_points, OneDimInterp::BSpline);
    color_ramp.write("demo/color_ramp.png");

    Image separate_channels(demo);
    std::vector<Image*> v = separate_channels.separate_channels();
    for (int i = 0; i < v.size(); i++) {
        v[i]->write(("demo/" + std::to_string(i) + "_separate_channel.png").c_str());
    }
    v[0]->f_scale(v[0]->w / 2, v[0]->h);

    Image combine_channels(demo);
    combine_channels.combine_channels(v);
    combine_channels.write("demo/combine_channel.png");

    Image set_alpha(demo);
    std::vector<std::pair<double, Color>> set_alpha_points{
        {0.2, Color(0, 0, 0)}, {0.4, Color(255, 255, 255)}, {0.8, Color(50, 50, 50)}};
    Image alpha_map = set_alpha.preview_color_ramp(set_alpha_points);
    set_alpha.set_alpha(alpha_map, true);
    set_alpha.write("demo/set_alpha.png");

    Image color_balance(demo);
    color_balance.color_balance(Color(0.3 * 255, 0.3 * 255, 0.2 * 255), Color(0.430 * 255, 0.18 * 255, 0.19 * 255),
                                Color(255, 255, 255));
    color_balance.write("demo/color_balance.png");

    Image histogram = demo.histogram();
    histogram.write("demo/histogram.png");

    Image HSV(demo);
    HSV.HSV(90, 0.4, -0.4);
    HSV.write("demo/HSV.png");

    Image false_color = demo.false_color();
    false_color.write("demo/false_color.png");

    Image tone_correct(demo);
    Adjustment s = Adjustment::create_adj_bc(40, 40);
    Adjustment h = Adjustment::create_adj_lgg(0.0, 1.0, 1.0);
    Adjustment m = Adjustment::create_adj_hsv(0, -1, 0.4);
    tone_correct.tone_correct(60, 170, s, m, h);
    tone_correct.write("demo/tone_correct.png");

    Image RGB_curves(demo);
    std::vector<std::pair<double, double>> rgb_curves_points1{{0.0, 0.0}, {0.4, 0.0}, {0.7, 0.5}, {0.75, 0.6},
                                                              {0.8, 0.7}, {0.9, 1.0}, {1.0, 1.0}};
    std::vector<std::pair<double, double>> rgb_curves_points2{{0.0, 0.0}, {0.4, 0.8}, {0.9, 0.5}, {1.0, 1.0}};
    Image preview_RGB_curves = RGB_curves.preview_RGB_curves(
        OneDimInterp::Bezier, rgb_curves_points1, rgb_curves_points2, rgb_curves_points1, rgb_curves_points2);
    preview_RGB_curves.write("demo/preview_RGB_curves.png");
    RGB_curves.RGB_curves(OneDimInterp::Bezier, rgb_curves_points1, rgb_curves_points2, rgb_curves_points1,
                          rgb_curves_points2);
    RGB_curves.write("demo/RGB_curves.png");

    Image hue_correct(demo);
    std::vector<std::pair<double, double>> control_h = {{179, -0.5}, {180, 0}, {181, 0.3}};
    std::vector<std::pair<double, double>> control_s = {{129, 0.5}, {130, 0}, {131, -0.3}};
    std::vector<std::pair<double, double>> control_v = {{269, -0.5}, {270, 0}, {271, 0.3}};
    Image preview_hue_correct = hue_correct.preview_hue_correct(control_h, control_s, control_v);
    preview_hue_correct.write("demo/preview_hue_correct.png");
    hue_correct.hue_correct(control_h, control_s, control_v);
    hue_correct.write("demo/hue_correct.png");

    Image blur(demo);
    blur.blur(Blur::Gaussian, 300, 5);
    blur.write("demo/blur.png");

    Image alpha_overlay(demo);
    std::vector<std::pair<double, Color>> alpha_overlay_color_ramp_points{
        {0.2, Color(0, 0, 0)}, {0.4, Color(255, 255, 255)}, {0.7, Color(0, 0, 0)}, {0.9, Color(255, 255, 255)}};
    Image alpha_overlay_color_ramp = alpha_overlay.preview_color_ramp(alpha_overlay_color_ramp_points);
    alpha_overlay_color_ramp.f_scale(alpha_overlay.w, alpha_overlay.h, false, TwoDimInterp::Bilinear);
    alpha_overlay.alpha_overlay(&alpha_overlay_color_ramp, 0, 0, &demo2, 0, 0);
    alpha_overlay.write("demo/alpha_overlay.png");

    Image vignette(demo);
    vignette.vignette(0.5, 0.5);
    vignette.write("demo/vignette.png");

    Image color_matrix(demo);
    std::vector<std::vector<double>> color_matrix_matrix = {{0.393, 0.349, 0.272, 0, 0},
                                                            {0.769, 0.686, 0.534, 0, 0},
                                                            {0.189, 0.168, 0.131, 0, 0},
                                                            {0, 0, 0, 1, 0},
                                                            {0, 0, 0, 0, 1}};
    color_matrix.color_matrix(color_matrix_matrix);
    color_matrix.write("demo/color_matrix.png");

    Image rect_mask(demo);
    Image ellipse_mask(demo);
    rect_mask.alpha_overlay(&rect_mask.rect_mask(100, 100, 700, 900), 0, 0, Color(255, 255, 255));
    ellipse_mask.alpha_overlay(&ellipse_mask.ellipse_mask(400, 600, 250, 500), 0, 0, Color(255, 255, 255));
    rect_mask.write("demo/rect_mask.png");
    ellipse_mask.write("demo/ellipse_mask.png");

    Image white_noise = demo.white_noise();
    white_noise.write("demo/white_noise.png");

    Image p_noise_1 = demo.perlin_noise(0, 255, 400, 1, 0.5, 0);
    Image p_noise_2 = demo.perlin_noise(0, 255, 400, 2, 0.25, 0);
    Image p_noise_3 = demo.perlin_noise(0, 255, 400, 4, 0.125, 0);
    Image p_noise_4 = demo.perlin_noise(0, 255, 400, 8, 0.0625, 0);
    p_noise_1.write("p_noise_1.png");
    for (int r = 0; r < p_noise_1.h; r++) {
        for (int c = 0; c < p_noise_1.w; c++) {
            double tmp = std::clamp(p_noise_1.get(r, c, 0) + p_noise_2.get(r, c, 0) + p_noise_3.get(r, c, 0) +
                                        p_noise_4.get(r, c, 0),
                                    0, 255);
            p_noise_1.set(r, c, 0, tmp);
            p_noise_1.set(r, c, 1, tmp);
            p_noise_1.set(r, c, 2, tmp);
        }
    }
    p_noise_1.write("demo/perlin_noise.png");
}

int main(int argc, char** argv) {

    // demo();

    // Image color_reduce("demo/original/demo.jpeg");
    // color_reduce.color_reduce(ColorDepth::Bit_3, true);
    // color_reduce.crop(700, 500, 100, 100);
    // color_reduce.write("color_reduce_with.png");

    // Image* ret = new Image(256, 20, 3);
    // for (int i = 0; i < ret->h; i++) {
    //     for (int j = 0; j < ret->w; j++) {
    //         for (int cd = 0; cd < 3; cd++) {
    //             ret->data[(i * ret->w + j) * ret->channels + cd] = j;
    //         }
    //     }
    // }
    // ret->false_color(true);
    // ret->write("demo/false_color_preview.png");

    // Image demo("demo/original/demo.jpeg");
    // Image demo2("demo/original/demo_2.jpeg");
    // Image demo3("demo/original/demo_3.png");
    //
    // Image greyscale_avg(demo);
    // greyscale_avg.grayscale_avg();
    // // greyscale_avg.write("demo/greyscale_avg.png");
    //
    // Image greyscale_lum(demo);
    // greyscale_lum.grayscale_lum();
    // // greyscale_lum.write("demo/greyscale_lum.png");
    //
    // Image color_mask(demo);
    // color_mask.color_mask(0.6, 1, 0.2);
    // // color_mask.write("demo/color_mask.png");
    //
    // Image diffmap(demo);
    // diffmap.diffmap(greyscale_avg);
    // diffmap.write("demo/diffmap.png");
    //
    // Image diffmap_scale(greyscale_lum);
    // diffmap_scale.diffmap_scale(greyscale_avg);
    // diffmap_scale.write("demo/diffmap_scale.png");
    // Image saturation("demo/original/demo.jpeg");
    // saturation.saturation(-1, 0.5);
    // saturation.saturation(1, 0.5);
    // saturation.saturation(2, 0.5);
    // saturation.write("demo/saturation.png");
    // Image tone_correct("demo/original/demo.jpeg");
    // Adjustment s = Adjustment::create_adj_bc(40, 40);
    // Adjustment h = Adjustment::create_adj_lgg(0.0, 1.0, 1.0);
    // Adjustment m = Adjustment::create_adj_hsv(0, -1, 0.4);
    // tone_correct.tone_correct(60, 170, s, m, h);
    // tone_correct.write("demo/tone_correct.png");
    // Image colorful("images/colorful.jpg");
    // Adjustment A = Adjustment::create_adj_lgg(0, 1, 1.5);
    // colorful.tone_correct(100, 200, A, A, A);
    // colorful.HSV(0, 1, 0);
    // colorful.contrast(0, 150);
    // colorful.contrast(1, 150);
    // colorful.contrast(2, 150);
    // colorful.write("adj.png");
    // Image colorful("images/colorful.jpg");
    // Image *pn[6];
    // double freq = 1, amp = 1;
    // for (int i = 0; i < 6; i++) {
    //     freq *= 2;
    //     amp /= 2;
    //     pn[i] = &colorful.perlin_noise(0, 255, 400, freq, amp);
    // }
    // for (int r = 0; r < colorful.h; r++) {
    //     for (int c = 0; c < colorful.w; c++) {
    //         double tmp = 0;
    //         for (int i = 0; i < 6; i++) {
    //             tmp +=pn[i]->get(r, c, 0);
    //         }
    //         colorful.set(r, c, 0, tmp);
    //         colorful.set(r, c, 1, tmp);
    //         colorful.set(r, c, 2, tmp);
    //     }
    // }
    // colorful.write("pn_octave.png");

    // Image p_noise_1 = colorful.perlin_noise(0, 255, 400, 1, 0.5, 0);
    // Image p_noise_2 = colorful.perlin_noise(0, 255, 400, 2, 0.25, 0);
    // Image p_noise_3 = colorful.perlin_noise(0, 255, 400, 4, 0.125, 0);
    // Image p_noise_4 = colorful.perlin_noise(0, 255, 400, 8, 0.0625, 0);
    // p_noise_1.write("p_noise_1.png");
    // for (int r = 0; r < p_noise_1.h; r++) {
    //     for (int c = 0; c < p_noise_1.w; c++) {
    //         double tmp = std::clamp(p_noise_1.get(r, c, 0) + p_noise_2.get(r, c, 0) + p_noise_3.get(r, c, 0) +
    //                                     p_noise_4.get(r, c, 0),
    //                                 0, 255);
    //         p_noise_1.set(r, c, 0, tmp);
    //         p_noise_1.set(r, c, 1, tmp);
    //         p_noise_1.set(r, c, 2, tmp);
    //     }
    // }
    // p_noise_1.write("p_noise_octave.png");
    // p_noise_4.write("p_noise_4.png");

    // Image white_noise = colorful.white_noise();
    // white_noise.write("white_noise.png");
    // Image mask = colorful.ellipse_mask(180, 200, 180, 80);
    // // Image mask = colorful.rect_mask(40, 40, 80, 80);
    // mask.write("ellipse_mask.png");
    // colorful.vignette(0.4, 0);
    // std::vector<std::vector<double>> matrix = {
    //     {0.393,0.349,0.272,0,0},
    //     {0.769,0.686,0.534,0,0},
    //     {0.189,0.168,0.131,0,0},
    //     {0, 0, 0, 1, 0},
    //     {0, 0, 0, 0, 1}
    // };
    // colorful.color_matrix(matrix);

    // colorful.write("color_matrix.png");

    // Image colorful("images/clear_bg.png");
    // Image colorful_rampped("images/colorful.jpg");
    // std::vector<std::pair<double, Color>> points{{0.4, Color(0, 0, 0)}, {0.7, Color(255, 255, 255)}};
    // colorful_rampped.color_ramp(points);
    // colorful_rampped.write("colorful_rampped.png");
    // Image fac("images/0_separate.png");
    // colorful.alpha_overlay(&fac, 300, 300, &colorful_rampped, 300, 300);

    // preview.color_reduce(ColorDepth::Bit_8, true);
    // colorful.color_reduce(ColorDepth::Bit_8, true);
    // preview.write("preview.png");
    // colorful.write("reduce_2.png");
    // colorful.blur(Blur::Gaussian, 400, 400);
    // colorful.saturation(-1, -1);
    // colorful.exposure(-1);

    // colorful.preview_hue_correct()->write("preview_hue_correct.png");
    // colorful.hue_correct();

    // std::vector<std::pair<double, double>> points{{0.0, 0.0}, {0.4, 0.0}, {0.7, 0.5}, {0.75, 0.6}, {0.8, 0.7},
    // {0.9, 1.0}, {1.0, 1.0}};
    // std::vector<std::pair<double, double>> points{{0.0, 0.0}, {0.4, 0.0}, {0.9, 1.0},
    // {1.0, 1.0}};
    //
    // colorful.RGB_curves(OneDimInterp::Bezier, points, points, points, points);

    // segfault here
    // Image* res = colorful.preview_RGB_curves(OneDimInterp::Bezier, points, points, points);

    // res->write("preview.png");
    // res[0]->write("images/colorful_c.png");
    // res[1]->write("images/colorful_r.png");
    // res[2]->write("images/colorful_g.png");
    // res[3]->write("images/colorful_b.png");

    // Image** res = colorful.preview_RGB_curves(OneDimInterp::Bezier, points);
    // res = colorful.preview_RGB_curves(OneDimInterp::BSpline, points);
    // res[0]->write("images/colorful_c_bspline.png");
    // Image hist_lum = colorful.histogram((uint32_t) 0);
    // hist_lum.write("images/hist_lum.png");

    // Image hue(720, 360, 3);
    // Color c;
    // for (int r = 0; r < hue.h; r++) {
    //     for (int w = 0; w < hue.w; w++) {
    //         c.hsv_to_rgb((int)round(w / 2.0 + (360 - r) + 180 + 360) % 360, 1, 1);
    //         hue.set(r, w, 0, c.r);
    //         hue.set(r, w, 1, c.g);
    //         hue.set(r, w, 2, c.b);
    //     }
    // }
    // hue.write("images/hue.png");
    //
    // Image sat(720, 360, 3);
    // for (int r = 0; r < sat.h; r++) {
    //     for (int w = 0; w < sat.w; w++) {
    //         c.hsv_to_rgb(w / 2.0, (360 - r) / 360.0, 1);
    //         sat.set(r, w, 0, c.r);
    //         sat.set(r, w, 1, c.g);
    //         sat.set(r, w, 2, c.b);
    //     }
    // }
    // sat.write("images/sat.png");
    //
    // Image vlu(720, 360, 3);
    // for (int r = 0; r < vlu.h; r++) {
    //     for (int w = 0; w < sat.w; w++) {
    //         c.hsv_to_rgb(w / 2.0, 1, (360 - r) / 360.0);
    //         vlu.set(r, w, 0, c.r);
    //         vlu.set(r, w, 1, c.g);
    //         vlu.set(r, w, 2, c.b);
    //     }
    // }
    // vlu.write("images/vlu.png");

    // Image bezier(200, 100, 3);
    //
    // std::vector<std::pair<double, double>> ctrl{{30, 40}, {60, 90}, {99, 90}, {120, 0}, {140, 30}, {199,
    // 40}};
    // std::vector<std::pair<double, double>> ctrl{
    //     {0, 10},   {40, 10},   {40, 80},  {60, 80},  {80, 80},
    //                                             {80, 50},  {100, 50},  {120, 50}, {120, 70},
    //                                                 {150, 70},
    //                                             {180, 70}, {180, 99}, {199, 99}};
    // std::vector<double> interp;
    // Interpolation I;
    // for (int i = 0; i < 200; i++) {
    //     interp.push_back(i);
    // }
    // std::vector<double> res = I.b_spline(ctrl, interp);
    //
    // for (int i = 0; i < 200; i++) {
    //     bezier.set(round(99 - res[i]), interp[i], 0, 255);
    //     std::cout << res[i] << "\n";
    // }
    // for (int i = 0; i < ctrl.size(); i++) {
    //     bezier.set(round(99 - ctrl[i].second), ctrl[i].first, 1, 255);
    // }
    //
    // bezier.write("images/bezier.png");

    // Color c = new Color(2, 0, 0);
    // Color c2 = new Color(100, 0, 0);
    // Color c3 = c + c2;

    // c->set(c2);
    // std::cout << c3.r << "\n";

    // Image colorful("images/colorful.jpg");
    // colorful.rotate(0, 0, 30);
    // Image hist = colorful.histogram(true, -1);
    // hist.write("hist.png");

    // Color c = colorful.get_color_or_default(-9, 10);
    // std::cout << c.r << "\n";

    // colorful.rotate(0, 0, 30);
    // colorful.write("colorful_rotate.png");

    // Color c(255,255, 255);
    // std::cout<< c.luminance() << "\n";
    // std::vector<std::pair<double, Color>> points{{0.0, Color(0, 0, 0)}, {1.0, Color(255, 255, 255)}};
    // Adjustment a = Adjustment();
    // Adjustment s = a.create_adj_bc(0, 0);
    // // Adjustment m = a.create_adj_bc(0, 0);
    // Adjustment m = a.create_adj_hsv(90, -2, 0);
    // Adjustment h = a.create_adj_bc(0, 0);
    // colorful.tone_correct(90, 190, s, m, h);
    // colorful.write("images/colorful_tone_correct.png");

    // Image preview = colorful.preview_color_ramp(points);
    // preview.write("images/color_ramp_preview.png");
    // preview.false_color(true);
    // colorful.false_color(true);
    // colorful.HSV(90, 0, 0);
    // colorful.HSV(-90, 0, 0);
    // colorful.color_balance(Color(0, 0, 0), Color(255, 255, 255), Color(255, 255, 255));
    // colorful.write("images/colorful_false_color.png");

    // Color c (10, 0.1, 20);
    // c = c.rgb_to_hsv(10, 0.1, 20);
    // printf("%f, %f, %f\n", c.r, c.g, c.b);
    // c.hsv_to_rgb(c.r, c.g, c.b);
    // printf("%f, %f, %f\n", c.r, c.g, c.b);

    // std::vector<std::pair<double, Color>> points{
    //     {0.0, Color(100.0, 100.0, 100.0)},
    //                                              {0.2, Color(100.0, 100.0, 100.0)},
    //                                              {0.4, Color(200.0, 200.0, 200.0)},
    //                                              {0.8, Color(50, 50, 50)},
    //                                              {0.9, Color(255, 255, 255)},
    //                                              {1.0, Color(255, 255, 255)}
    // };
    // std::vector<std::pair<double, Color>> points{{0.0, Color(0.0, 0.0, 0.0)}, {1.0, Color(255.0, 255.0, 255.0)}};
    // // std::sort(points.begin(), points.end());
    // Image colorful("images/test1.jpg");
    // // colorful.translate(-100, 100, Color(255, 0, 0, 100));
    // // colorful.write("images/test1_translated.png");
    // Image bezier = colorful.preview_color_ramp(points, OneDimInterp::Bezier);
    // Image linear = colorful.preview_color_ramp(points, OneDimInterp::Linear);
    // Image bspline = colorful.preview_color_ramp(points, OneDimInterp::BSpline);
    // Image graph(256, 256, 3);
    //
    // for (int i =0; i < 256; i++) {
    //     graph.set(preview.get(0, i), i, 0, 255);
    //     graph.set(preview.get(0, i), i, 1, 255);
    //     graph.set(preview.get(0, i), i, 2, 255);
    // }
    // graph.write("images/color_ramp_graph.png");
    // bezier.write("images/color_ramp_preview_bezier.png");
    // linear.write("images/color_ramp_preview_linear.png");
    // bspline.write("images/color_ramp_preview_bspline.png");
    // preview.f_scale(1000, 1000, false, TwoDimInterp::Bilinear);
    // preview.write("images/color_ramp_preview_scale.png");

    // Image colorful("images/colorful.jpg");
    // Image hist = colorful.histogram(false);
    // hist.write("images/histogram.png");

    // Color a(100, 100, 100, 100);
    // Color b(20, 100, 30, 40);
    // Color c = a + b;
    // std::cout << c.a << "\n";

    // colorful.color_balance(Color(0.756*255/2, 0.604*255/2, 0.409*255/2), Color(0.870*255/2, 0.371*255/2,
    // 0.382*255/2)
    //         , Color(255,255,255));
    //         // , Color(2*255, 2*255, 2*255));
    // colorful.write("images/colorful_balance.png");
    // Image clear_bg("images/clear_bg.png");
    // std::vector<std::pair<double, Color>> points{
    //     {0.2, Color(0, 0, 0)}, {0.4, Color(255, 255, 255)}, {0.8, Color(50, 50, 50)}};
    // std::vector<Image*> v = colorful.seperate_channels();
    // Image color_ramp = colorful.preview_color_ramp(points);
    // clear_bg.set_alpha(color_ramp, true);
    // clear_bg.write("clear_bg_set_alpha.png");
    // colorful.set_alpha(color_ramp, true);
    // color_ramp.write("color_ramp_f_scale.png");
    // for (int i = 0; i < v.size(); i++) {
    //     v[i]->write((std::to_string(i) + "_separate.png").c_str());
    // }
    // v[0]->f_scale(v[0]->w/2, v[0]->h);
    // colorful.combine_channels(v);
    // colorful.write("combined.png");

    // int max = -INFINITY;
    // max = fmax(1, max);
    // std::cout << max << "\n";

    // std::vector<std::pair<double, Color>> points{
    //     {0.2, Color(0, 100, 200)}, {0.4, Color(100, 100, 100)}, {0.8, Color(200, 0, 100)}};
    // std::vector<std::pair<double, Color>> points{
    //     {0.0, Color(255, 0, 0)}, {1.0, Color(0, 0, 255)}};
    // std::sort(points.begin(), points.end());
    // Image colorful("images/test1.jpg");
    // colorful.translate(-100, 100, Color(255, 0, 0, 100));
    // colorful.write("images/test1_translated.png");
    // preview.write("images/color_ramp_preview.png");

    // std::sort(points.begin(), points.end());
    // for (int i = 0; i < points.size(); i++) {
    //     printf("(%f, [%f, %f, %f]),\n", points[i].first, points[i].second.r, points[i].second.g,
    //     points[i].second.b);
    // }
    //
    // std::cout << (std::upper_bound(points.begin(), points.end(), std::make_pair(0.0, Color(110, 110, 110))) -
    // points.begin())<< "\n";

    // Image colorful("images/test1.jpg");
    // colorful.color_reduce(true);
    // colorful.write("images/test1_cr.png");

    // Image colorful("images/colorful.jpg");
    // Image colorful_near("images/colorful.jpg");
    // colorful.f_scale(1200, -1, true, Bilinear);
    // colorful.write("images/colorful_f_scale.png");
    // colorful_near.f_scale(1200, -1, true, Nearest);
    // colorful_near.write("images/colorful_f_scale_near.png");
    // colorful_near.diffmap_scale(colorful);
    // colorful_near.write("images/colorful_f_scale_diff.png");

    // colorful.edge();
    // Color c(100, .44, .44);
    // c.hsv_to_rgb(100, .44, .44);
    // printf("%f, %f, %f", c.r, c.g, c.b);

    // test1();
}
