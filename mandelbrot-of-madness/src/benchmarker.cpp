#include <iostream>
#include <complex>
#include <string>
#include <thread>
#include <numeric>
#include <chrono>

#include <SFML/Graphics/Image.hpp>

#define CL_TARGET_OPENCL_VERSION 220
#include <CL/cl.h>

void singlethreaded(int width, int height, double resolution, int iterations) {
    std::complex<double> pivot;

    double *plane = new double[width * height];

    for (int y = 0; y < height; y++) {
        double imag = pivot.imag() + ((y - (float) height / 2.0f) / resolution);

        for (int x = 0; x < width; x++) {
            double real = pivot.real() + ((x - (float) width / 2.0f) / resolution);

            std::complex<double> c = std::complex<double>(real, imag);
            std::complex<double> z;
            int i = 0;
            for (; i < iterations && std::norm(z) < 2; i++) {
                z = z * z + c;
            }

            plane[y * width + x] = (double) i / (double) iterations;
        }
    }

    sf::Image image = sf::Image(sf::Vector2u(width, height), sf::Color(0, 0, 0));

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            unsigned char color = 255.0f - plane[y * width + x] * 255.0f;
            image.setPixel(sf::Vector2u(x, y), sf::Color(color, color, color));
        }
    }

    delete[] plane;
}

void CalculateMandelbrot(double *plane, int width, int height, double resolution, int iterations, std::complex<double> pivot, int from, int to) {
    for (int i = from; i < to; i++) {
        int y = i / width;
        int x = i % width;

        double imag = pivot.imag() + ((y - (float) height / 2.0f) / resolution);
        double real = pivot.real() + ((x - (float) width / 2.0f) / resolution);

        std::complex<double> c = std::complex<double>(real, imag);
        std::complex<double> z;
        
        int iter = 0;
        for (; iter < iterations && std::norm(z) < 2; iter++) {
            z = z * z + c;
        }

        plane[y * width + x] = (double) iter / (double) iterations;
    }
}

void multithreaded(int width, int height, double resolution, int iterations) {
    std::complex<double> pivot;

    double *plane = new double[width * height];

    int threadcount = std::thread::hardware_concurrency();

    int pointsPerChunk = std::lcm(sizeof(double), 256) / sizeof(double);
    int chunks = width * height / pointsPerChunk;

    if (chunks == 0) {
        for (int y = 0; y < height; y++) {
            double imag = pivot.imag() + ((y - (float) height / 2.0f) / resolution);
    
            for (int x = 0; x < width; x++) {
                double real = pivot.real() + ((x - (float) width / 2.0f) / resolution);
    
                std::complex<double> c = std::complex<double>(real, imag);
                std::complex<double> z;
                int i = 0;
                for (; i < iterations && std::norm(z) < 2; i++) {
                    z = z * z + c;
                }
    
                plane[y * width + x] = (double) i / (double) iterations;
            }
        }
    } else {
        int chunksPerThread = chunks / threadcount;
        int residualChunks = chunks % threadcount;

        std::thread *threads = new std::thread[threadcount];

        int i = 256 - ((int) plane % 256);
        i += sizeof(double) - ((i - (int) plane) % sizeof(double));
        i += chunksPerThread;

        if (residualChunks > 0) {
            i += pointsPerChunk;
            residualChunks--;
        }

        threads[0] = std::thread(CalculateMandelbrot, plane, width, height, resolution, iterations, pivot, 0, i);

        for (int t = 1; t < threadcount - 1; t++) {
            int j = i + chunksPerThread * pointsPerChunk;

            if (residualChunks > 0) {
                j += pointsPerChunk;
                residualChunks--;
            }

            threads[t] = std::thread(CalculateMandelbrot, plane, width, height, resolution, iterations, pivot, i, j);

            i = j;
        }

        threads[threadcount - 1] = std::thread(CalculateMandelbrot, plane, width, height, resolution, iterations, pivot, i, width * height);

        for (int t = 0; t < threadcount; t++) {
            threads[t].join();
        }

        delete[] threads;
    }

    sf::Image image = sf::Image(sf::Vector2u(width, height), sf::Color(0, 0, 0));

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            unsigned char color = 255.0f - plane[y * width + x] * 255.0f;
            image.setPixel(sf::Vector2u(x, y), sf::Color(color, color, color));
        }
    }

    delete[] plane;
}

const char mandelbrotKernelSource[] = R"(
__kernel void generate_mandelbrot(int2 dimensions, float resolution, int iterations, float2 pivot, __global uchar4 *out) {
    int x = get_global_id(0);
    int y = get_global_id(1);

    if (x >= dimensions.x || y >= dimensions.y) return;

    float2 z = pivot + ((float2)(x, y) - (float2)(dimensions.x, dimensions.y) / 2) / resolution;
    float2 c = z;

    int iter = 0;
    for (int i = 0; i < iterations; i++) {
        float xx = z.x * z.x;
        float yy = z.y * z.y;

        if (xx + yy > 4.0f) break;

        z = (float2)(xx - yy, 2 * z.x * z.y) + c;

        iter = i;
    }
    
    float lerp = 1.0f - (float)(iter) / (float)(iterations);
    float3 color = (float3)(255.0f, 255.0f, 255.0f) * lerp;
    
    out[y * dimensions.x + x] = (uchar4)(color.x, color.y, color.z, 255);
}
)";

void gpuaccel(int width, int height, double resolution, int iterations) {
    const char *source = mandelbrotKernelSource;
    size_t sourceLength = sizeof(mandelbrotKernelSource);
    int dimensions[2] = {width, height};
    size_t szDimensions[2] = {width, height};
    float pivot[2] = {0, 0};

    cl_int clError;
    cl_uint platformCount;
    cl_platform_id *platforms;
    cl_device_id device;
    cl_context context;
    cl_command_queue commandQueue;
    cl_program program;
    cl_kernel kernel;
    cl_mem buffer;

    clError = clGetPlatformIDs(0, nullptr, &platformCount);
    if (clError != CL_SUCCESS) {
        std::cout << "An error occured when trying to count platforms!\n";
        return;
    }

    platforms = new cl_platform_id[platformCount];
    clError = clGetPlatformIDs(platformCount, platforms, &platformCount);
    if (clError != CL_SUCCESS) {
        std::cout << "An error occured when trying to query platforms!\n";
        delete[] platforms;
        return;
    }
    
    for (int i = 0; i < platformCount; i++) {
        clError = clGetDeviceIDs(platforms[i], CL_DEVICE_TYPE_DEFAULT, 1, &device, nullptr);
        if (clError == CL_SUCCESS) break;
    }

    if (device == NULL) {
        std::cout << "An error occured when trying to obtain OpenCL device!\n";
        delete[] platforms;
        return;
    }

    context = clCreateContext(0, 1, &device, nullptr, nullptr, &clError);
    if (clError != CL_SUCCESS) {
        std::cout << "An error occured when trying to create context!\n";
        delete[] platforms;
        return;
    }

    commandQueue = clCreateCommandQueue(context, device, 0, &clError);
    if (clError != CL_SUCCESS) {
        std::cout << "An error occured when trying to create command queue!\n";
        clReleaseContext(context);
        delete[] platforms;
        return;
    }

    program = clCreateProgramWithSource(context, 1, &source, &sourceLength, &clError);
    if (clError != CL_SUCCESS) {
        std::cout << "An error occured when trying to create program!\n";
        clReleaseCommandQueue(commandQueue);
        clReleaseContext(context);
        delete[] platforms;
        return;
    }

    clError = clBuildProgram(program, 1, &device, nullptr, nullptr, nullptr);
    if (clError != CL_SUCCESS) {
        std::cout << "An error occured when trying to build program!\n";

        size_t len;
        char buffer[2048];

        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, sizeof(buffer), buffer, &len);

        std::cout << buffer << "\n";

        clReleaseProgram(program);
        clReleaseCommandQueue(commandQueue);
        clReleaseContext(context);
        delete[] platforms;
        return;
    }

    kernel = clCreateKernel(program, "generate_mandelbrot", &clError);
    if (clError != CL_SUCCESS) {
        std::cout << "An error occured when trying to create kernel!\n";
        clReleaseProgram(program);
        clReleaseCommandQueue(commandQueue);
        clReleaseContext(context);
        delete[] platforms;
        return;
    }

    buffer = clCreateBuffer(context, CL_MEM_READ_WRITE, width * height * sizeof(cl_uchar4), nullptr, &clError);
    if (clError != CL_SUCCESS) {
        std::cout << "An error occured when trying to create buffer!\n";
        clReleaseKernel(kernel);
        clReleaseProgram(program);
        clReleaseCommandQueue(commandQueue);
        clReleaseContext(context);
        delete[] platforms;
        return;
    }

    clError = clSetKernelArg(kernel, 0, sizeof(cl_int2), dimensions);
    clError |= clSetKernelArg(kernel, 1, sizeof(cl_float), &resolution);
    clError |= clSetKernelArg(kernel, 2, sizeof(cl_int), &iterations);
    clError |= clSetKernelArg(kernel, 3, sizeof(cl_float2), pivot);
    clError |= clSetKernelArg(kernel, 4, sizeof(cl_mem), &buffer);
    if (clError != CL_SUCCESS) {
        std::cout << "An error occured when trying to set kernel arguments!\n";
        clReleaseMemObject(buffer);
        clReleaseKernel(kernel);
        clReleaseProgram(program);
        clReleaseCommandQueue(commandQueue);
        clReleaseContext(context);
        delete[] platforms;
        return;
    }

    clError = clEnqueueNDRangeKernel(commandQueue, kernel, 2, nullptr, szDimensions, nullptr, 0, nullptr, nullptr);
    if (clError != CL_SUCCESS) {
        std::cout << "An error occured when trying to enqueue work!\n";
        clReleaseMemObject(buffer);
        clReleaseKernel(kernel);
        clReleaseProgram(program);
        clReleaseCommandQueue(commandQueue);
        clReleaseContext(context);
        delete[] platforms;
        return;
    }

    uint8_t *pixelData = new uint8_t[width * height * 4];

    clError = clEnqueueReadBuffer(commandQueue, buffer, CL_TRUE, 0, sizeof(uint8_t) * width * height * 4, pixelData, 0, nullptr, nullptr);
    if (clError != CL_SUCCESS) {
        std::cout << "An error occured when trying to enqueue read!\n";
        delete[] pixelData;
        clReleaseMemObject(buffer);
        clReleaseKernel(kernel);
        clReleaseProgram(program);
        clReleaseCommandQueue(commandQueue);
        clReleaseContext(context);
        delete[] platforms;
        return;
    }

    delete[] pixelData;
    clReleaseMemObject(buffer);
    clReleaseKernel(kernel);
    clReleaseProgram(program);
    clReleaseCommandQueue(commandQueue);
    clReleaseContext(context);
    delete[] platforms;
}

using Clock = std::chrono::high_resolution_clock;

int main(int argc, char **argv) {
    std::pair<int, int> dimensions[] = {
        {640, 360},
        {1280, 720},
        {1920, 1080},
        {2560, 1440},
        {3840, 2160},
        {7680, 4320}
    };

    double resolutions[] = {100, 200, 400, 800};

    int iterations[] = {100, 200, 400, 800};

    for (auto [width, height] : dimensions) {
        for (double resolution : resolutions) {
            for (int iteration : iterations) {
                std::cout << "Dimension: " << width << "x" << height << " ";
                std::cout << "Resolution: " << resolution << " ";
                std::cout << "Iterations: " << iteration << "\n";

                auto start = Clock::now();
                singlethreaded(width, height, resolution, iteration);
                auto end = Clock::now();

                std::cout << "- Singlethreaded : " << std::chrono::duration<double, std::milli>(end - start).count() << "ms\n";

                start = Clock::now();
                multithreaded(width, height, resolution, iteration);
                end = Clock::now();

                std::cout << "- Multithreaded : " << std::chrono::duration<double, std::milli>(end - start).count() << "ms\n";

                start = Clock::now();
                gpuaccel(width, height, resolution, iteration);
                end = Clock::now();

                std::cout << "- GPU Accelerated : " << std::chrono::duration<double, std::milli>(end - start).count() << "ms\n\n";
            }
        }
    }
}