cmake_minimum_required(VERSION 3.10)

# Project name
project(Image)

# Specify C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Find SDL2 and SDL2_image packages
find_package(SDL2 REQUIRED)
find_package(SDL2_image REQUIRED)
find_package(SDL2_ttf REQUIRED)

# Add the executable
add_executable(Image src/main.cpp)  # Replace 'main.cpp' with your source file

# Link SDL2 and SDL2_image libraries
target_link_libraries(Image SDL2::SDL2 SDL2_image::SDL2_image SDL2_ttf::SDL2_ttf)

