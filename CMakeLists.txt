cmake_minimum_required(VERSION 3.14)
project(SoftwareRadioLinkSimulator VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(srls main.cpp)

# Link statically against libstdc++ and libgcc for this specific target
target_link_options(srls PRIVATE -static-libstdc++ -static-libgcc -fopenmp)
if (MINGW AND CMAKE_CXX_COMPILER_ID STREQUAL GNU)
    target_link_libraries(srls stdc++exp)
endif()

