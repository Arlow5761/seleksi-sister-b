#include <iostream>
#include <complex>
#include <string>

#include <SFML/Graphics/Image.hpp>

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

    if (!image.saveToFile(filepath)) {
        std::cout << "An Error Occured!\n";
        return -1;
    }

    std::cout << "Successfully generated image";
    return 0;
}