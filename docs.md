# Node definitions

## I/O
```
io::image 
{
    src: string,
}
-> image
```
- `src`: source path of image

```
image ->
io::output 
{
    dest: string,
}
```
- `dest`: output path of image

## Spacial operations
```
image -> 
spacial::crop 
{
    start_x: number,
    start_y: number,
    width: number,
    height: number,
}
-> image
```
- `start_x`: positive number; x coordinate of crop,
- `start_y`: positive number; y coordinate of crop,
- `width`: positive number; width of final image
- `height`: positive number; height of final image

```
image -> 
spacial::scale
{
    width: number,
    height: number,
    linked: bool,
    method: string,
}
-> image
```
- `width`: positive number; width of final image
- `height`: positive number; height of final image
- `linked`: whether to keep original image proportions
- `method`: interpretation method to use. Supported methods are "nearest" and "bilinear"

## Statistics
```
image -> 
stats::histogram 
{
    (include_lum: bool),
}
-> preview
```
- `include_lum`: whether to include the overall luminosity levels

## Color
```
image ->
color::color_ramp
{
    interp_method: string,
    control_points: list,
}
-> preview
-> image
```
- `interp_method`: interpretation method. Supported methods are "constant", "linear", "bspline", and
"bezier".
- `control_points`: list of elements in the form `[ number ∈ [0, 1], color ]`. 

```
image_1 ->
image_2 ->
(factor ->)
color::mix 
{
    (factor: number),
    (fac_x: number = 0),
    (fac_y: number = 0),
    (second_x: number = 0),
    (second_y: number = 0),
}
-> image
```
- `factor`: required if `factor` image is not provided
- `fac_x`: the x coordinate of factor image over the first image
- `fac_y`: the y coordinate of factor image over the first image
- `second_x`: the x coordinate of second image overlaying the first image
- `second_y`: the y coordinate of second image overlaying the first image

```
image -> 
color::grayscale
{
    (method: string),
}
-> image
```
- `method`: `"lum"` or `"avg"`; optionally specify whether to use luminosity or average sampling method, defaults to luminosity




<!-- ``` -->
<!-- node::image -->
<!-- { -->
<!--     src: *src_of_image*, -->
<!-- }; -->
<!-- -> output image -->
<!---->
<!-- Node::mix -->
<!-- { -->
<!--     fac: *factor*, -->
<!-- }; -->
<!-- -> output image -->
<!---->
<!-- Node::color_ramp -->
<!-- { -->
<!--     method: *interp_method*, -->
<!--     control_points: *list_of_control_points, -->
<!-- }; -->
<!-- -> output image -->
<!-- -> preview image -->
<!---->
<!-- Node::output -->
<!-- { -->
<!--     path: *path*, -->
<!-- }; -->
<!-- ``` -->
