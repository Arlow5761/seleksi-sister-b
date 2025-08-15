cmake_minimum_required(VERSION 4.0)

project(mandelbrot-of-madness VERSION 1.0)

set(CMAKE_CXX_STANDARD 17)

find_package(SFML COMPONENTS Graphics System Window CONFIG REQUIRED)
find_package(OpenCL CONFIG REQUIRED)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin/")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin/")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin/")

add_executable(singlethreaded ${CMAKE_SOURCE_DIR}/src/singlethreaded.cpp)
target_link_libraries(singlethreaded PRIVATE SFML::Graphics SFML::System)

add_executable(multithreaded ${CMAKE_SOURCE_DIR}/src/multithreaded.cpp)
target_link_libraries(multithreaded PRIVATE SFML::Graphics SFML::System )

add_executable(gpu-accel ${CMAKE_SOURCE_DIR}/src/gpu-accel.cpp)
target_link_libraries(gpu-accel PRIVATE SFML::Graphics SFML::System OpenCL::OpenCL OpenCL::Headers OpenCL::HeadersCpp OpenCL::Utils OpenCL::UtilsCpp)

add_executable(benchmarker ${CMAKE_SOURCE_DIR}/src/benchmarker.cpp)
target_link_libraries(benchmarker PRIVATE SFML::Graphics SFML::System OpenCL::OpenCL OpenCL::Headers OpenCL::HeadersCpp OpenCL::Utils OpenCL::UtilsCpp)

add_executable(gui ${CMAKE_SOURCE_DIR}/src/gui.cpp)
target_link_libraries(gui PRIVATE SFML::Graphics SFML::Window SFML::System)