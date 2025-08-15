# Mandelbrot Of Madness
Mandelbrot fractal generator using singlethreaded, multithreaded, and gpu acceleration implementations.

Singlethreaded : Full for loops
Multithreaded : Each thread handles a range of points
GPU Accelerated : Kernel runs on each pixel at once

## Prerequisite
1. vcpkg
2. cmake
3. OpenCL Drivers

## How to build
```cmake --preset default```
```cmake --build --preset default```

## How to run
Run the executable in the bin directory after building

## Showcase
https://drive.google.com/file/d/1Wr7qYkIAyKHUhfzwEIEfDN51_ktw5kcc/view?usp=drive_link

## GUI Controls
Plus : Increase Iterations

Minus : Decrease Iterations

Drag : Pan

M : Switch between Mandelbrot and Julia

L : Lock C value for Julia
