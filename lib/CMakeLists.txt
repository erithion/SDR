add_library(sdrlib INTERFACE)

# Specify the include directories for users of this library
target_include_directories(sdrlib INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/inc
)

# Optionally, list the headers to make them visible in IDEs (CMake 3.23+ feature)
# target_sources(sdrlib INTERFACE 
#    ${CMAKE_CURRENT_SOURCE_DIR}/include/sdrlib/sdrlib.hpp
# )

add_subdirectory(tests)
