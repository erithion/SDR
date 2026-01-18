# Include the FetchContent module
include(FetchContent)

# Specify and download GoogleTest
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG v1.17.0 # Use a specific stable release tag
)

FetchContent_MakeAvailable(googletest)

add_executable(sdrlib_test fft_test.cpp ofdm_test.cpp sliding_buffer_test.cpp)

# Link the test executable to the header-only library target
target_link_libraries(sdrlib_test PRIVATE sdrlib GTest::gmock GTest::gtest_main)

# Register the test with CTest
add_test(NAME sdrlib_run_test COMMAND sdrlib_test)

#include(GoogleTest)
#gtest_discover_tests(sdr_test)
