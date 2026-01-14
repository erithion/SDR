cmake_minimum_required(VERSION 3.14)
project(SDR VERSION 0.1 DESCRIPTION "A software-defined radio stack" LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

enable_testing()
add_subdirectory(lib)
add_subdirectory(app)