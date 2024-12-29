; ModuleID = 'Eleminima module'
source_filename = "Eleminima module"

@str = private unnamed_addr constant [68 x i8] c"/Users/tigerding/Projects/eleminima/runtime/demo/original/demo.jpeg\00", align 1
@str.1 = private unnamed_addr constant [46 x i8] c"/Users/tigerding/Projects/eleminima/demo.jpeg\00", align 1

declare void @image_grayscale_avg(ptr)

declare ptr @image_create_from_file(ptr)

declare i1 @image_write(ptr, ptr)

define void @main() {
entry:
  %0 = call ptr @image_create_from_file(ptr @str)
  call void @image_grayscale_avg(ptr %0)
  %1 = call i1 @image_write(ptr %0, ptr @str.1)
  ret void
}
