cmake_minimum_required(VERSION 3.16)

project("C++Renderer" LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Keep the vendored MiniFB build focused on the renderer executable.
set(MINIFB_BUILD_EXAMPLES OFF CACHE BOOL "Build MiniFB examples" FORCE)
set(MINIFB_BUILD_VERSION_INFO OFF CACHE BOOL "Build MiniFB version utility" FORCE)
set(MINIFB_USE_OPENGL_API OFF CACHE BOOL "Use MiniFB's OpenGL backend" FORCE)
add_subdirectory(minifb)

# GLM is header-only. Expose it as a normal CMake target without building its tests.
add_library(glm_headers INTERFACE)
add_library(glm::glm ALIAS glm_headers)
target_include_directories(glm_headers INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/glm")

file(GLOB_RECURSE RENDERER_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/*.hpp"
)

add_executable(C++Renderer ${RENDERER_SOURCES})
target_include_directories(C++Renderer PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)
target_link_libraries(C++Renderer PRIVATE minifb::minifb glm::glm)

if(WIN32)
    target_link_libraries(C++Renderer PRIVATE winmm user32 gdi32)
endif()

if(MSVC)
    target_compile_options(C++Renderer PRIVATE /W4 /permissive-)
else()
    target_compile_options(C++Renderer PRIVATE -Wall -Wextra -Wpedantic)
endif()
