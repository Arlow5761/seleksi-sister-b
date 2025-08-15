#include <iostream>
#include <string>

#include <SFML/Graphics/Image.hpp>

#define CL_TARGET_OPENCL_VERSION 220
#include <CL/cl.h>

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

int main(int argc, char **argv) {
    int width;
    int height;
    float resolution;
    int iterations;
    std::string filepath;
    
    std::cout << "Enter width: ";
    std::cin >> width;

    std::cout << "Enter height: ";
    std::cin >> height;

    std::cout << "Enter resolution: ";
    std::cin >> resolution;

    std::cout << "Enter iterations: ";
    std::cin >> iterations;

    std::cout << "Enter output filepath: ";
    std:: cin >> filepath;

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
        return -1;
    }

    platforms = new cl_platform_id[platformCount];
    clError = clGetPlatformIDs(platformCount, platforms, &platformCount);
    if (clError != CL_SUCCESS) {
        std::cout << "An error occured when trying to query platforms!\n";
        delete[] platforms;
        return -1;
    }
    
    for (int i = 0; i < platformCount; i++) {
        clError = clGetDeviceIDs(platforms[i], CL_DEVICE_TYPE_DEFAULT, 1, &device, nullptr);
        if (clError == CL_SUCCESS) break;
    }

    if (device == NULL) {
        std::cout << "An error occured when trying to obtain OpenCL device!\n";
        delete[] platforms;
        return -1;
    }

    context = clCreateContext(0, 1, &device, nullptr, nullptr, &clError);
    if (clError != CL_SUCCESS) {
        std::cout << "An error occured when trying to create context!\n";
        delete[] platforms;
        return -1;
    }

    commandQueue = clCreateCommandQueue(context, device, 0, &clError);
    if (clError != CL_SUCCESS) {
        std::cout << "An error occured when trying to create command queue!\n";
        clReleaseContext(context);
        delete[] platforms;
        return -1;
    }

    program = clCreateProgramWithSource(context, 1, &source, &sourceLength, &clError);
    if (clError != CL_SUCCESS) {
        std::cout << "An error occured when trying to create program!\n";
        clReleaseCommandQueue(commandQueue);
        clReleaseContext(context);
        delete[] platforms;
        return -1;
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
        return -1;
    }

    kernel = clCreateKernel(program, "generate_mandelbrot", &clError);
    if (clError != CL_SUCCESS) {
        std::cout << "An error occured when trying to create kernel!\n";
        clReleaseProgram(program);
        clReleaseCommandQueue(commandQueue);
        clReleaseContext(context);
        delete[] platforms;
        return -1;
    }

    buffer = clCreateBuffer(context, CL_MEM_READ_WRITE, width * height * sizeof(cl_uchar4), nullptr, &clError);
    if (clError != CL_SUCCESS) {
        std::cout << "An error occured when trying to create buffer!\n";
        clReleaseKernel(kernel);
        clReleaseProgram(program);
        clReleaseCommandQueue(commandQueue);
        clReleaseContext(context);
        delete[] platforms;
        return -1;
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
        return -1;
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
        return -1;
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
        return -1;
    }

    sf::Image output = sf::Image(sf::Vector2u(width, height), pixelData);

    if (!output.saveToFile(filepath)) {
        std::cout << "An error occured when trying to save image!\n";
        delete[] pixelData;
        clReleaseMemObject(buffer);
        clReleaseKernel(kernel);
        clReleaseProgram(program);
        clReleaseCommandQueue(commandQueue);
        clReleaseContext(context);
        delete[] platforms;
        return -1;
    }

    std::cout << "Successfully generated image";
    delete[] pixelData;
    clReleaseMemObject(buffer);
    clReleaseKernel(kernel);
    clReleaseProgram(program);
    clReleaseCommandQueue(commandQueue);
    clReleaseContext(context);
    delete[] platforms;
    return 0;
}