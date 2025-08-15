#include <iostream>
#include <complex>
#include <string>
#include <thread>
#include <numeric>

#include <SFML/Graphics/Image.hpp>

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

int main(int argc, char **argv) {
    int width;
    int height;
    double resolution;
    int iterations;
    std::complex<double> pivot;
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

        delete threads;
    }

    sf::Image image = sf::Image(sf::Vector2u(width, height), sf::Color(0, 0, 0));

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            unsigned char color = 255.0f - plane[y * width + x] * 255.0f;
            image.setPixel(sf::Vector2u(x, y), sf::Color(color, color, color));
        }
    }

    if (!image.saveToFile(filepath)) {
        std::cout << "An Error Occured!\n";
        return -1;
    }

    std::cout << "Successfully generated image";
    return 0;
}