#!/bin/bash

# Check if CMake is installed
if ! command -v cmake &> /dev/null; then
    echo "Error: CMake is not installed. Install it with:"
    echo "  sudo apt update && sudo apt install -y cmake"
    exit 1
fi

# Check for CMakeLists.txt
if [ ! -f "CMakeLists.txt" ]; then
    echo "Error: CMakeLists.txt not found in the current directory."
    exit 1
fi

# Set up build directory
BUILD_DIR="build"
if [ ! -d "$BUILD_DIR" ]; then
    mkdir "$BUILD_DIR"
fi

cd "$BUILD_DIR" || exit 1

# Configure and build
echo "==> Configuring CMake..."
cmake .. || { echo "CMake failed"; exit 1; }


echo "==> Building project..."
make -j$(nproc) || { echo "Build failed"; exit 1; }

# Find and run the executable (assumes first target in CMakeLists.txt)
EXECUTABLE="Image"
if [ -z "$EXECUTABLE" ]; then
    echo "Error: Could not detect executable name from CMakeLists.txt"
    exit 1
fi

echo "==> Running ./$EXECUTABLE..."
./"$EXECUTABLE"
