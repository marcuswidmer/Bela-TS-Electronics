cmake_minimum_required(VERSION 3.10)
project(SamplerTests)

# Specify C++ standard
set(CMAKE_CXX_STANDARD 17)

# Add the main library (Sampler)
add_library(Sampler ../Sampler.cpp)
target_include_directories(Sampler PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)

# Find Google Test package
find_package(GTest REQUIRED)
enable_testing()

# Add the test executable
add_executable(SamplerTest SamplerTest.cpp)
target_link_libraries(SamplerTest PRIVATE Sampler GTest::GTest GTest::Main)

# Add the test to CTest
add_test(NAME SamplerTests COMMAND SamplerTest)