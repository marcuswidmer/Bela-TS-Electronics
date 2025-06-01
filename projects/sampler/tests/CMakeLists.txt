cmake_minimum_required(VERSION 3.10)
project(SamplerTests)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 17)

add_definitions(-DUSE_MAC_SNDFILE)

# Find Google Test
find_package(GTest REQUIRED)
enable_testing()

# Add the main project code (the library you're testing)
add_library(Sampler ../Sampler.cpp)
add_library(Voice ../Voice.cpp)
add_library(ADSR ../ADSR.cpp)
add_library(AudioFile ../../../libraries/AudioFile/AudioFile.cpp)
add_library(AudioFileUtilities ../../../libraries/AudioFile/AudioFileUtilities.cpp)

# Find libsndfile
# Find libsndfile
#find_package(SndFile REQUIRED)

# Add include directories for libsndfile
target_include_directories(Voice PRIVATE /opt/homebrew/include)
target_include_directories(Sampler PRIVATE /opt/homebrew/include)
target_include_directories(AudioFile PRIVATE /opt/homebrew/include)

target_link_libraries(AudioFile PRIVATE /opt/homebrew/lib/libsndfile.dylib)

target_include_directories(AudioFileUtilities PRIVATE /opt/homebrew/include)
target_link_libraries(AudioFileUtilities PRIVATE /opt/homebrew/lib/libsndfile.dylib)

target_link_libraries(Sampler PRIVATE AudioFile)
target_link_libraries(Sampler PRIVATE AudioFileUtilities)
target_link_libraries(Voice PRIVATE ADSR)



# Add the libraries/Midi directory to the include path
target_include_directories(Sampler PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_include_directories(Sampler PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../../include)
target_include_directories(Sampler PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../..)
target_include_directories(Voice PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../..)
target_include_directories(Sampler PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../..)

# Add the test executable
add_executable(SamplerTest SamplerTest.cpp)
target_compile_definitions(SamplerTest PRIVATE USE_NO_MIDI)
target_compile_definitions(Sampler PRIVATE USE_NO_MIDI)

# Link the main project code and GTest libraries to the test executable
target_link_libraries(SamplerTest PRIVATE Sampler GTest::GTest GTest::Main)
target_link_libraries(SamplerTest PRIVATE Voice GTest::GTest GTest::Main)
target_link_libraries(SamplerTest PRIVATE AudioFile GTest::GTest GTest::Main)

# Register the test executable with CTest
add_test(NAME SamplerTests COMMAND SamplerTest)