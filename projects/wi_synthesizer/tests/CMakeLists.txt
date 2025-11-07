cmake_minimum_required(VERSION 3.10)
project(WiSynthesizerTests)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 17)

add_definitions(-DUSE_MAC_SNDFILE)

# Find Google Test
find_package(GTest REQUIRED)
enable_testing()

# Add the main project code (the library you're testing)
add_library(WiSynthesizer ../WiSynthesizer.cpp)
add_library(ADSR ../ADSR.cpp)
add_library(GranularReverb ../GranularReverb.cpp)
add_library(Ramp ../Ramp.cpp)

# Find libsndfile
# Find libsndfile
#find_package(SndFile REQUIRED)

# Add include directories for libsndfile
target_include_directories(WiSynthesizer PRIVATE /opt/homebrew/include)

target_link_libraries(WiSynthesizer PRIVATE ADSR)
target_link_libraries(WiSynthesizer PRIVATE GranularReverb)
target_link_libraries(GranularReverb PRIVATE Ramp)

# Add the libraries/Midi directory to the include path
target_include_directories(WiSynthesizer PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_include_directories(WiSynthesizer PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../../include)
target_include_directories(WiSynthesizer PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../..)
target_include_directories(WiSynthesizer PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../..)

# Add the test executable
add_executable(WiSynthesizerTest WiSynthesizerTest.cpp)
target_compile_definitions(WiSynthesizerTest PRIVATE USE_NO_MIDI)
target_compile_definitions(WiSynthesizer PRIVATE USE_NO_MIDI)

# Link the main project code and GTest libraries to the test executable
target_link_libraries(WiSynthesizerTest PRIVATE WiSynthesizer GTest::GTest GTest::Main)

# Register the test executable with CTest
add_test(NAME WiSynthesizerTests COMMAND WiSynthesizerTest)