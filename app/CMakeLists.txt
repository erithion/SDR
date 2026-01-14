add_executable(sdr main.cpp)

# Link the executable to the header-only library target
target_link_libraries(sdr PRIVATE sdrlib)

target_link_options(sdr PRIVATE -static-libstdc++ -static-libgcc) #-fopenmp)
if (MINGW AND CMAKE_CXX_COMPILER_ID STREQUAL GNU)
    target_link_libraries(sdr stdc++exp)
endif()
