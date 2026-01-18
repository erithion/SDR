find_package(Qt6 REQUIRED COMPONENTS Widgets PrintSupport)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

include(FetchContent)

# 1. Declare the source (using the latest stable 2.1.1 link)
FetchContent_Declare(
    qcustomplot
    URL https://www.qcustomplot.com/release/2.1.1/QCustomPlot-source.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE  # Add this line
)

# 2. Download and extract
FetchContent_MakeAvailable(qcustomplot)

# 3. Define the library using the fetched files
# FetchContent provides ${qcustomplot_SOURCE_DIR} automatically
add_library(qcustomplot STATIC 
    ${qcustomplot_SOURCE_DIR}/qcustomplot.cpp 
    ${qcustomplot_SOURCE_DIR}/qcustomplot.h
)

# 4. Link dependencies and set includes
target_link_libraries(qcustomplot PUBLIC Qt6::Widgets Qt6::PrintSupport)
target_include_directories(qcustomplot INTERFACE ${qcustomplot_SOURCE_DIR})

add_executable(sdr main.cpp QFDemoWindow.cpp QFDemoWindow.hpp)
target_link_libraries(sdr PRIVATE sdrlib Qt6::Widgets qcustomplot)
target_link_options(sdr PRIVATE -static-libstdc++ -static-libgcc)

if (MINGW AND CMAKE_CXX_COMPILER_ID STREQUAL GNU)
    target_link_libraries(sdr PRIVATE stdc++exp)
endif()
