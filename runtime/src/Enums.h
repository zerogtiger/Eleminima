#ifndef ENUMS_H
#define ENUMS_H

enum ImageType { PNG, JPG, BMP, TGA };
enum TwoDimInterp { Nearest, Bilinear, Bicubic, Fourier, Edge, HQX, Mipmap };
enum OneDimInterp { Constant, Linear, Bezier, BSpline };
enum Blur { Box, Gaussian, Bokeh };
enum ColorDepth { Bit_3, Bit_8, Bit_16, Bit_24 };

#endif
