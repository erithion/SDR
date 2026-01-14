
add_executable(sdr main.cpp)

include(FetchContent)

FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.91.4 # Use a specific version tag
)
FetchContent_MakeAvailable(imgui)

# Fetch ImPlot
FetchContent_Declare(
    implot
    GIT_REPOSITORY https://github.com/epezent/implot
    GIT_TAG        v0.16 # Use latest stable version
)
FetchContent_MakeAvailable(implot)

FetchContent_Declare(
  glfw
  GIT_REPOSITORY https://github.com/glfw/glfw
  GIT_TAG 3.4 # Consider using a specific release tag (e.g., '3.3.8')
)

# Make the content available and build it
FetchContent_MakeAvailable(glfw)

find_package(OpenGL REQUIRED)

target_sources(sdr PRIVATE
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp

    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp

    ${implot_SOURCE_DIR}/implot.cpp
    ${implot_SOURCE_DIR}/implot_items.cpp
)

# ------------------------------------------------------------
# Include paths (merged & cleaned)
# ------------------------------------------------------------
target_include_directories(sdr PRIVATE
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
    ${implot_SOURCE_DIR}
)

# ------------------------------------------------------------
# Link real libraries ONLY
# ------------------------------------------------------------
target_link_libraries(sdr PRIVATE
    sdrlib         # your own library
    glfw
    OpenGL::GL
)

# ------------------------------------------------------------
# Static runtime (MinGW)
# ------------------------------------------------------------
target_link_options(sdr PRIVATE -static-libstdc++ -static-libgcc)

if (MINGW AND CMAKE_CXX_COMPILER_ID STREQUAL GNU)
    target_link_libraries(sdr PRIVATE stdc++exp)
endif()
