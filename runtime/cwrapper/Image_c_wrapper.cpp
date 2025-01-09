#include "../src/Image.h"
#include <cstdint>
#include <iostream>

extern "C" {

typedef enum {
    ONE_DIM_INTERP_CONSTANT,
    ONE_DIM_INTERP_LINEAR,
    ONE_DIM_INTERP_BEZIER,
    ONE_DIM_INTERP_BSPLINE,
} OneDimInterpC;

typedef enum {
    TWO_DIM_INTERP_NEAREST,
    TWO_DIM_INTERP_BILINEAR,
    TWO_DIM_INTERP_BICUBIC,
    TWO_DIM_INTERP_FOURIER,
    TWO_DIM_INTERP_EDGE,
    TWO_DIM_INTERP_HQX,
    TWO_DIM_INTERP_MIPMAP,
} TwoDimInterpC;
}

OneDimInterp map_one_dim_interp(OneDimInterpC method)
{
    switch (method) {
    case ONE_DIM_INTERP_CONSTANT:
        return OneDimInterp::Constant;
    case ONE_DIM_INTERP_LINEAR:
        return OneDimInterp::Linear;
    case ONE_DIM_INTERP_BEZIER:
        return OneDimInterp::Bezier;
    case ONE_DIM_INTERP_BSPLINE:
        return OneDimInterp::BSpline;
    default:
        return OneDimInterp::Linear; // Fallback
    }
}

TwoDimInterp map_two_dim_interp(TwoDimInterpC method)
{
    switch (method) {
    case TWO_DIM_INTERP_NEAREST:
        return TwoDimInterp::Nearest;
    case TWO_DIM_INTERP_BILINEAR:
        return TwoDimInterp::Bilinear;
    case TWO_DIM_INTERP_BICUBIC:
        return TwoDimInterp::Bicubic;
    case TWO_DIM_INTERP_FOURIER:
        return TwoDimInterp::Fourier;
    case TWO_DIM_INTERP_EDGE:
        return TwoDimInterp::Edge;
    case TWO_DIM_INTERP_HQX:
        return TwoDimInterp::HQX;
    case TWO_DIM_INTERP_MIPMAP:
        return TwoDimInterp::Mipmap;
    default:
        return TwoDimInterp::Nearest; // Fallback
    }
}

extern "C" {

typedef struct ImageWrapper {
    void* instance;
} ImageWrapper;

typedef struct ColorWrapper {
    void* instance;
} ColorWrapper;

typedef struct PointColorPair {
    double value;
    ColorWrapper* color;
} PointColorPair;

ColorWrapper* color_create_default() { return new ColorWrapper{new Color{}}; }

ColorWrapper* color_create_rgb(double r, double g, double b)
{
    return new ColorWrapper{new Color{r, g, b}};
}

ColorWrapper* color_create_rgba(double r, double g, double b, double a)
{
    return new ColorWrapper{new Color{r, g, b, a}};
}

void color_destroy(ColorWrapper* color) { delete reinterpret_cast<Color*>(color->instance); }

ImageWrapper* image_create_w_h_channels(int w, int h, int channels)
{
    return new ImageWrapper{new Image(w, h, channels)};
}

ImageWrapper* image_create_w_h_channels_fill(int w, int h, int channels, ColorWrapper* fill)
{
    return new ImageWrapper{new Image(w, h, channels, *reinterpret_cast<Color*>(fill))};
}

ImageWrapper* image_create_filename(const char* filename)
{
    return new ImageWrapper{new Image(filename)};
}

void image_write(ImageWrapper* img, const char* filename)
{
    if (!reinterpret_cast<Image*>(img->instance)->write(filename)) {
        std::cout << "Failed to write image to filename \"" << filename << "\", unsupported format";
    }
}

// Destructor Wrapper
void image_destroy(ImageWrapper* img) { delete reinterpret_cast<Image*>(img->instance); }

// uint8_t image_get_pixel(ImageWrapper* img, uint32_t row, uint32_t col, uint32_t channel) {
//     return reinterpret_cast<Image*>(img->instance)->get(row, col, channel);
// }
//
// bool image_set_pixel(ImageWrapper* img, uint32_t row, uint32_t col, uint32_t channel, uint8_t
// value) {
//     return reinterpret_cast<Image*>(img->instance)->set(row, col, channel, value);
// }

// Grayscale example
void image_grayscale_avg(ImageWrapper* img)
{
    reinterpret_cast<Image*>(img->instance)->grayscale_avg();
}

void image_grayscale_lum(ImageWrapper* img)
{
    reinterpret_cast<Image*>(img->instance)->grayscale_lum();
}

void image_crop(ImageWrapper* img, double cx, double cy, double cw, double ch)
{
    reinterpret_cast<Image*>(img->instance)
        ->crop((uint16_t) cx, (uint16_t) cy, (uint16_t) cw, (uint16_t) ch);
}

void image_f_scale(ImageWrapper* img, uint32_t new_w, uint32_t new_h, bool linked,
                   TwoDimInterpC method)
{
    reinterpret_cast<Image*>(img->instance)
        ->f_scale(new_w, new_h, linked, map_two_dim_interp(method));
}

ImageWrapper* image_histogram(ImageWrapper* img, bool inc_lum)
{
    return new ImageWrapper{
        &reinterpret_cast<Image*>(img->instance)->histogram(inc_lum, -1, Color(125, 125, 125))};
}

void image_color_ramp(ImageWrapper* img, PointColorPair* points, size_t points_count,
                      OneDimInterpC method)
{
    std::vector<std::pair<double, Color>> point_vec;
    for (size_t i = 0; i < points_count; ++i) {
        point_vec.push_back(
            {points[i].value, *reinterpret_cast<Color*>(points[i].color->instance)});
    }
    reinterpret_cast<Image*>(img->instance)->color_ramp(point_vec, map_one_dim_interp(method));
}

ImageWrapper* image_preview_color_ramp(ImageWrapper* img, PointColorPair* points,
                                       size_t points_count, OneDimInterpC method)
{
    std::vector<std::pair<double, Color>> point_vec;
    for (size_t i = 0; i < points_count; ++i) {
        point_vec.push_back(
            {points[i].value, *reinterpret_cast<Color*>(points[i].color->instance)});
    }
    return new ImageWrapper{&reinterpret_cast<Image*>(img->instance)
                                 ->preview_color_ramp(point_vec, map_one_dim_interp(method))};
}

void image_alpha_overlay_img_img(ImageWrapper* img, ImageWrapper* fac, int fac_x, int fac_y,
                                 ImageWrapper* other, int other_x, int other_y)
{
    reinterpret_cast<Image*>(img->instance)
        ->alpha_overlay(reinterpret_cast<Image*>(fac->instance), fac_x, fac_y,
                        reinterpret_cast<Image*>(other->instance), other_x, other_y);
}

void image_alpha_overlay_color_img(ImageWrapper* img, ColorWrapper* color, ImageWrapper* other,
                                   int other_x, int other_y)
{
    reinterpret_cast<Image*>(img->instance)
        ->alpha_overlay(*reinterpret_cast<Color*>(color->instance),
                        reinterpret_cast<Image*>(other->instance), other_x, other_y);
}

void image_alpha_overlay_color_color(ImageWrapper* img, ColorWrapper* color, ColorWrapper* other)
{
    reinterpret_cast<Image*>(img->instance)
        ->alpha_overlay(*reinterpret_cast<Color*>(color->instance),
                        *reinterpret_cast<Color*>(other));
}

void image_alpha_overlay_img_color(ImageWrapper* img, ImageWrapper* fac, int fac_x, int fac_y,
                                   ColorWrapper* other)
{
    reinterpret_cast<Image*>(img->instance)
        ->alpha_overlay(reinterpret_cast<Image*>(fac->instance), fac_x, fac_y,
                        *reinterpret_cast<Color*>(other));
}
} // extern C
