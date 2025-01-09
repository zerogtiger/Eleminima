; ModuleID = 'Eleminima module'
source_filename = "Eleminima module"

@str = private unnamed_addr constant [68 x i8] c"/Users/tigerding/Projects/eleminima/runtime/demo/original/demo.jpeg\00", align 1
@str.2 = private unnamed_addr constant [50 x i8] c"/Users/tigerding/Projects/eleminima/demo_avg.jpeg\00", align 1

declare ptr @color_create_default()

declare ptr @color_create_rgb(double, double, double)

declare ptr @color_create_rgba(double, double, double, double)

declare void @color_destroy(ptr)

declare ptr @image_create_w_h_channels(i32, i32, i32)

declare ptr @image_create_w_h_channels_fill(i32, i32, i32, ptr)

declare ptr @image_create_from_file(ptr)

declare i1 @image_write(ptr, ptr)

declare void @image_destroy(ptr)

declare void @image_grayscale_avg(ptr)

declare void @image_grayscale_lum(ptr)

declare void @image_crop(ptr, double, double, double, double)

declare void @image_f_scale(ptr, i32, i32, i1, i32)

declare ptr @image_histogram(ptr, i1)

declare void @image_color_ramp(ptr, ptr, i32, i32)

declare ptr @image_color_ramp.1(ptr, ptr, i32, i32)

declare void @image_alpha_overlay_img_img(ptr, ptr, i32, i32, ptr, i32, i32)

declare void @image_alpha_overlay_color_img(ptr, ptr, ptr, i32, i32)

declare void @image_alpha_overlay_color_color(ptr, ptr, ptr)

declare void @image_alpha_overlay_img_color(ptr, ptr, i32, i32, ptr)

define void @main() {
entry:
  %0 = call ptr @image_create_from_file(ptr @str)
  call void @image_grayscale_avg(ptr %0)
  %1 = call i1 @image_write(ptr %0, ptr @str.2)
  ret void
}
