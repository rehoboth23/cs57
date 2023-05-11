; ModuleID = 'test_llvm/test1.c'
source_filename = "test_llvm/test1.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

; Function Attrs: mustprogress noinline nounwind optnone uwtable
define dso_local noundef i32 @_Z4funci(i32 noundef %0) #0 {
  %2 = alloca i32, align 4
  %3 = alloca i32, align 4
  %4 = alloca i32, align 4
  %5 = alloca i32, align 4
  %6 = alloca i32, align 4
  %7 = alloca i32, align 4
  store i32 %0, ptr %3, align 4
  store i32 10, ptr %4, align 4
  store i32 20, ptr %5, align 4
  store i32 20, ptr %5, align 4
  store i32 20, ptr %5, align 4
  store i32 20, ptr %5, align 4
  store i32 5, ptr %5, align 4
  %8 = load i32, ptr %4, align 4
  %9 = load i32, ptr %5, align 4
  %10 = add nsw i32 %8, %9
  store i32 %10, ptr %6, align 4
  %11 = load i32, ptr %4, align 4
  %12 = load i32, ptr %5, align 4
  %13 = add nsw i32 %11, %12
  store i32 %13, ptr %7, align 4
  %14 = load i32, ptr %4, align 4
  %15 = icmp slt i32 %14, 10
  br i1 %15, label %16, label %24

16:                                               ; preds = %1
  %17 = load i32, ptr %3, align 4
  store i32 %17, ptr %4, align 4
  %18 = load i32, ptr %4, align 4
  %19 = load i32, ptr %5, align 4
  %20 = add nsw i32 %18, %19
  store i32 %20, ptr %7, align 4
  store i32 20, ptr %5, align 4
  %21 = load i32, ptr %5, align 4
  %22 = load i32, ptr %4, align 4
  %23 = add nsw i32 %21, %22
  store i32 %23, ptr %2, align 4
  br label %26

24:                                               ; preds = %1
  %25 = load i32, ptr %6, align 4
  store i32 %25, ptr %2, align 4
  br label %26

26:                                               ; preds = %24, %16
  %27 = load i32, ptr %2, align 4
  ret i32 %27
}

attributes #0 = { mustprogress noinline nounwind optnone uwtable "frame-pointer"="all" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }

!llvm.module.flags = !{!0, !1, !2, !3, !4}
!llvm.ident = !{!5}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 7, !"PIC Level", i32 2}
!2 = !{i32 7, !"PIE Level", i32 2}
!3 = !{i32 7, !"uwtable", i32 2}
!4 = !{i32 7, !"frame-pointer", i32 2}
!5 = !{!"Ubuntu clang version 15.0.7"}
