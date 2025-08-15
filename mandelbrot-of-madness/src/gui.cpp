#include <iostream>
#include <string>
#include <optional>

#include <SFML/Graphics/Image.hpp>
#include <SFML/Graphics/Shader.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/RectangleShape.hpp>

const char mandelbrotShaderSource[] = R"(
#version 110

uniform vec2 u_dimensions;
uniform float u_resolution;
uniform int u_iterations;
uniform vec2 u_pivot;
uniform vec2 u_origin;

void main() {
    vec2 c = u_pivot + (gl_FragCoord.xy - u_dimensions / vec2(2.0, 2.0)) / vec2(u_resolution, u_resolution);
    vec2 z = c;

    int iter = 0;
    for (int i = 0; i < u_iterations; i++) {
        if (dot(z, z) > 2.0) break;
        z = vec2(z.x * z.x - z.y * z.y, 2.0 * z.x * z.y) + c;
        iter = i;
    }
    
    vec3 color = mix(vec3(1.0, 1.0, 1.0), vec3(0.0, 0.0, 0.0), float(iter) / float(u_iterations));

    gl_FragColor = vec4(color, 1.0);
}
)";

const char juliaShaderSource[] = R"(
#version 110

uniform vec2 u_dimensions;
uniform float u_resolution;
uniform int u_iterations;
uniform vec2 u_pivot;
uniform vec2 u_origin;

void main() {
    vec2 z = u_pivot + (gl_FragCoord.xy - u_dimensions / vec2(2.0, 2.0)) / vec2(u_resolution, u_resolution);
    vec2 c = u_origin;

    int iter = 0;
    for (int i = 0; i < u_iterations; i++) {
        if (dot(z, z) > 4.0) break;
        z = vec2(z.x * z.x - z.y * z.y, 2.0 * z.x * z.y) + c;
        iter = i;
    }
    
    vec3 color = mix(vec3(1.0, 1.0, 1.0), vec3(0.0, 0.0, 0.0), float(iter) / float(u_iterations));

    gl_FragColor = vec4(color, 1.0);
}
)";

int main(int argc, char **argv) {
    int width = 800;
    int height = 600;
    float resolution = 256;
    int iterations = 200;
    sf::Vector2f pivot;
    
    bool julia = false;
    bool locked = false;
    sf::Vector2f origin;

    bool rerender = true;
    bool dragged = false;
    sf::Vector2i lastMousePos;

    sf::Shader mandelbrotShader = sf::Shader(std::string_view(mandelbrotShaderSource), sf::Shader::Type::Fragment);
    sf::Shader juliaShader = sf::Shader(std::string_view(juliaShaderSource), sf::Shader::Type::Fragment);
    sf::Shader *p_shader = &mandelbrotShader;

    sf::RectangleShape surface = sf::RectangleShape(sf::Vector2f(width, height));
    sf::RenderWindow window = sf::RenderWindow(sf::VideoMode(sf::Vector2u(width, height)), "Mandelbrot Set Viewer");

    while (window.isOpen()) {
        while (const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
            }
            
            if (const auto *mousePressed = event->getIf<sf::Event::MouseButtonPressed>()) {
                if (mousePressed->button == sf::Mouse::Button::Left) {
                    dragged = true;
                    lastMousePos = mousePressed->position;
                }
            }

            if (const auto *mouseReleased = event->getIf<sf::Event::MouseButtonReleased>()) {
                if (mouseReleased->button == sf::Mouse::Button::Left) {
                    dragged = false;
                }
            }

            if (const auto *mouseLeft = event->getIf<sf::Event::MouseLeft>()) {
                dragged = false;
            }

            if (const auto *mouseMoved = event->getIf<sf::Event::MouseMoved>()) {
                if (dragged) {
                    pivot += sf::Vector2f(lastMousePos.x - mouseMoved->position.x, mouseMoved->position.y - lastMousePos.y) / resolution;
                    rerender = true;
                } else if (!locked){
                    origin += sf::Vector2f(lastMousePos.x - mouseMoved->position.x, mouseMoved->position.y - lastMousePos.y) / resolution;

                    if (julia) {
                        rerender = true;
                    }
                }

                lastMousePos = mouseMoved->position;
            }

            if (const auto *scrolled = event->getIf<sf::Event::MouseWheelScrolled>()) {
                float oldResolution = resolution;
                resolution *= std::powf(2, scrolled->delta);

                if (resolution < 1.0f) {
                    resolution = 1.0f;
                }
                

                if (!locked) {
                    origin = pivot + (origin - pivot) * resolution / oldResolution;
                }

                rerender = true;
            }

            if (const auto *resized = event->getIf<sf::Event::Resized>()) {
                width = resized->size.x;
                height = resized->size.y;

                origin = sf::Vector2f(width / 2 - lastMousePos.x, lastMousePos.y - height / 2) / resolution;

                rerender = true;
            }

            if (const auto *keyPressed = event->getIf<sf::Event::KeyPressed>()) {
                switch (keyPressed->code) {
                    // Increase iteration count
                    case sf::Keyboard::Key::Equal:
                        iterations += 100;

                        rerender = true;
                    break;

                    // Decrease iteration count
                    case sf::Keyboard::Key::Subtract:
                        iterations -= 100;

                        if (iterations < 100) {
                            iterations = 100;
                        }

                        rerender = true;
                    break;

                    // Change modes
                    case sf::Keyboard::Key::M:
                        julia = !julia;

                        if (julia) {
                            p_shader = &juliaShader;
                        } else {
                            p_shader = &mandelbrotShader;
                        }

                        rerender = true;
                    break;

                    // Lock value of C
                    case sf::Keyboard::Key::L:
                        locked = !locked;

                        if (!locked) {
                            origin = sf::Vector2f(width / 2 - lastMousePos.x, lastMousePos.y - height / 2) / resolution;
                            rerender = true;
                        }
                    break;
                }
            }
        }

        if (rerender) {
            p_shader->setUniform("u_dimensions", sf::Glsl::Vec2(width, height));
            p_shader->setUniform("u_resolution", resolution);
            p_shader->setUniform("u_iterations", iterations);
            p_shader->setUniform("u_pivot", pivot);
            p_shader->setUniform("u_origin", origin);
            
            surface.setSize(sf::Vector2f(width, height));
            surface.setPosition(sf::Vector2f(0, 0));

            window.clear();
            window.draw(surface, p_shader);
            window.display();

            rerender = false;
        }
    }

    return 0;
}