cmake_minimum_required(VERSION 3.14)
project(sdrtests CXX)

# Include the FetchContent module
include(FetchContent)

# Specify and download GoogleTest
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG v1.17.0 # Use a specific stable release tag
)

FetchContent_MakeAvailable(googletest)

# Add a test executable
add_executable(sdr_test fft_test.cpp)

# Link the test executable to your library and the GTest libraries
target_link_libraries(sdr_test PRIVATE GTest::gmock GTest::gtest_main)
target_include_directories(sdr_test PRIVATE ${CMAKE_SOURCE_DIR}/src)

# Enable testing and discover tests
enable_testing()
include(GoogleTest)
gtest_discover_tests(sdr_test)
