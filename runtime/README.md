# Tiny-Pixels Engine

A C++ graphics compositing library.

## See [docs.md](docs.md) for documentation

## What Tiny-Pixels can do

![Demo](demo/original/compiled.png)

## To-do
- Color
    - [x] Invert
    - [x] Histogram
        - [ ] Read on histogram equalization for new techniques of adjustment
    - [x] Hue correct
    - [x] Tonal correction
    - [x] Color balance (lift, gamma, gain)
    - [x] Saturation
    - [x] Exposure
    - [x] Gamma
    - [x] HSV adjustments
    - [x] RGB curves
        - [ ] Catmull-Rom spline
        - [ ] Eyedropper for black, white, gray levels
    - [ ] Tone map
    - [ ] White balance
        - [ ] Eyedropper
    - [x] False color
    - [ ] Mixing options
        - [x] Alpha over
        - [x] Separate colors (RGBA channels)
        - [x] Combine colors (RGBA channels)
        - [x] Color ramp
            - [x] Constant
            - [x] Linear
            - [x] Bézier
            - [x] B-Spline
                - [ ] Read on De Boor's for possible optimization
- Filtering
    - [ ] Color reduce
        - [x] 3-bit color
        - [x] 3-bit Floyd-Steinberg error diffusion
        - [x] 8-bit color
        - [x] 8-bit Floyd-Steinberg error diffusion
        - [x] 16-bit color
        - [x] 16-bit Floyd-Steinberg error diffusion
        - [ ] 8/16-bit color quantization
    - [ ] Blur
        - [x] Box
        - [x] Gaussian
        - [ ] Bokeh
    - [ ] Anti-aliasing
    - [x] Masking
    - [x] Vignette
    - [x] Color matrix
- Transform
    - [x] Rotate
    - [ ] Scale
        - [x] Nearest
        - [x] Bilinear
        - [ ] Bicubic
        - [ ] Fourier
        - [ ] Edge
        - [ ] HQX
        - [ ] Mipmap
    - [x] Translate
    - [x] Crop
    - [x] Flip
    - [ ] Lens distortion
- Misc.
    - [ ] Frame
    - [x] Text overlay
    - [ ] Procedural texture generation
        - [x] White noise
        - [x] (Improved) Perlin noise
        - [ ] Brick
        - [ ] Gradient
        - [ ] Musgrave
        - [ ] Simplex noise
        - [ ] Voronoi
        - [ ] Wave
- Drawing
    - [ ] Lines (unuseful)
    - [x] Rectangles
        - [ ] Support percentage of image
    - [x] Ellipse
- Documentation (on-going)

## High Priority Enhancements/Bug Fixes
- [x] Color ramp linear and b-spline interpolation shows weird red when using monotone control points
- [ ] Replace interpolation in code with dedicated function from `Interpolation` class
- [x] Bilinear scaling (`Image::preview_color_ramp` then scale to a large image via `Image::f_scale` with `TwoDimInterp::Biliea` as interpolation method
- [x] Refactor `Adjustment`, `Color` and `Font` from `image.h`

## Medium Priority Enhancements/Bug Fixes
- [ ] Standardize `Color` class function design
- [ ] Optimize memory for `Image::perlin_noise` (overuse of array memory for gradient vectors)
- [ ] Different interpolation methods for each channel for RGB curves
- [ ] Show area instead of line in RGB curve preview
- [ ] Show control points in RGB curve preview
- [ ] Single channel image support for rgb curves
- [x] Merge preview of RGB curves to one image
- [ ] Replace retrieving and setting data with dedicated get and set functions from `Image` class
- [ ] Test `Color::apply_adj_rgb` thoroughly for hue, saturation, value, lift, gamma, gain
- [ ] Support separated channel images in `Image::HSV`
- [ ] Color class HSV integration
- [ ] Demo images in `README.md`
- [ ] Ensure uniformity of logic when using data from another image (first channel vs. average grayscale vs. separated channel image)
- [ ] Unnecessary memory use/leaks
- [x] Rename `enum` to generalize for 1-D and 2-D interpolation methods

## Low Priority Enhancements/Bug Fixes
- [ ] Clean up code in `Image::false_color` function
- [ ] Use `uint_t` and `size_t` where necessary
- [ ] Edge bleeding in edge detection via `Image::edge()` function due to Fast Fourier transform limitations
- [ ] Sobel edge detection gradient for single channel images
- [ ] Make makefile build/run more efficient
- [x] Refactor `src/` and move library files into `src/lib/`

