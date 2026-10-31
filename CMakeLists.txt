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
# Vendored TinyXML-2 11.0.0; no download is needed when configuring/building.
add_library(tinyxml2 STATIC third_party/tinyxml2/tinyxml2.cpp)
target_include_directories(tinyxml2 PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/third_party/tinyxml2")
target_link_libraries(C++Renderer PRIVATE minifb::minifb glm::glm tinyxml2)

# Refresh the runtime scene when the source XML changes.
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/scene/Scene01.xml"
    "${CMAKE_CURRENT_BINARY_DIR}/scene/Scene01.xml" COPYONLY)
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/scene/CornellBox.xml"
    "${CMAKE_CURRENT_BINARY_DIR}/scene/CornellBox.xml" COPYONLY)
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/scene/Scene02.xml"
    "${CMAKE_CURRENT_BINARY_DIR}/scene/Scene02.xml" COPYONLY)

if(WIN32)
    target_link_libraries(C++Renderer PRIVATE winmm user32 gdi32)
endif()

if(MSVC)
    target_compile_options(C++Renderer PRIVATE /W4 /permissive-)
else()
    target_compile_options(C++Renderer PRIVATE -Wall -Wextra -Wpedantic)
endif()

include(CTest)
if(BUILD_TESTING)
    add_executable(SceneLoaderTests
        tests/SceneLoaderTests.cpp
        source/SceneLoader.cpp
        source/Scene.cpp
        source/SceneObject.cpp
        source/Camera.cpp
        source/Triangle.cpp
        source/Sphere.cpp
        source/Disk.cpp
        source/Light.cpp
    )
    target_include_directories(SceneLoaderTests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")
    target_link_libraries(SceneLoaderTests PRIVATE glm::glm tinyxml2)
    add_test(NAME SceneLoaderTests COMMAND SceneLoaderTests
        "${CMAKE_CURRENT_SOURCE_DIR}/scene/Scene01.xml"
        "${CMAKE_CURRENT_BINARY_DIR}/scene-loader-test-input.xml"
        "${CMAKE_CURRENT_SOURCE_DIR}/scene/CornellBox.xml"
        "${CMAKE_CURRENT_SOURCE_DIR}/scene/Scene02.xml")
endif()
