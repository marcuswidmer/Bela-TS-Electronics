#!/bin/sh

# Stop the script if any command fails
set -e

# Define the build directory
BUILD_DIR="build"

# Print a message to indicate the start of the process
echo "Cleaning and rebuilding the project..."

# Remove the build directory if it exists
if [ -d "$BUILD_DIR" ]; then
    echo "Removing existing build directory: $BUILD_DIR"
    rm -rf "$BUILD_DIR"
else
    echo "No existing build directory found."
fi

# Create a fresh build directory
echo "Creating a new build directory: $BUILD_DIR"
mkdir "$BUILD_DIR"

# Navigate into the build directory
cd "$BUILD_DIR"

# Run CMake to configure the project
echo "Configuring the project with CMake..."
cmake ..

# Build the project using Make
echo "Building the project..."
make

# Print a success message
echo "Build completed successfully!"