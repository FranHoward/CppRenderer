cmake_minimum_required(VERSION 3.16)

project("C++Renderer" LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(RENDERER_ENABLE_OPENGL "Build the optional OpenGL realtime backend" ON)

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
if(NOT RENDERER_ENABLE_OPENGL)
    list(FILTER RENDERER_SOURCES EXCLUDE REGEX "/(OpenGLRenderer|GLShader|Glad)\\.cpp$")
endif()

add_executable(C++Renderer ${RENDERER_SOURCES})
target_include_directories(C++Renderer PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)
# Vendored TinyXML-2 11.0.0; no download is needed when configuring/building.
add_library(tinyxml2 STATIC third_party/tinyxml2/tinyxml2.cpp)
target_include_directories(tinyxml2 PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/third_party/tinyxml2")
target_link_libraries(C++Renderer PRIVATE minifb::minifb glm::glm tinyxml2)

if(RENDERER_ENABLE_OPENGL)
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
    set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
    add_subdirectory(third_party/glfw EXCLUDE_FROM_ALL)
    find_package(OpenGL REQUIRED)
    target_include_directories(C++Renderer SYSTEM PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/third_party/glfw/deps")
    target_link_libraries(C++Renderer PRIVATE glfw OpenGL::GL)
    target_compile_definitions(C++Renderer PRIVATE RENDERER_ENABLE_OPENGL=1
        RENDERER_SHADER_DIR="${CMAKE_CURRENT_BINARY_DIR}/shaders")
    foreach(SHADER scene.vert scene.frag shadow.vert shadow.frag)
        configure_file("${CMAKE_CURRENT_SOURCE_DIR}/shaders/${SHADER}"
            "${CMAKE_CURRENT_BINARY_DIR}/shaders/${SHADER}" COPYONLY)
    endforeach()
endif()

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

    add_executable(SceneGpuAdapterTests
        tests/SceneGpuAdapterTests.cpp
        source/SceneGpuAdapter.cpp
        source/SceneLoader.cpp
        source/Scene.cpp
        source/SceneObject.cpp
        source/Camera.cpp
        source/Triangle.cpp
        source/Sphere.cpp
        source/Disk.cpp
        source/Light.cpp)
    target_include_directories(SceneGpuAdapterTests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")
    target_link_libraries(SceneGpuAdapterTests PRIVATE glm::glm tinyxml2)
    add_test(NAME SceneGpuAdapterTests COMMAND SceneGpuAdapterTests
        "${CMAKE_CURRENT_SOURCE_DIR}/scene/CornellBox.xml")

    find_package(Threads REQUIRED)
    add_executable(RendererShutdownTests
        tests/RendererShutdownTests.cpp source/Renderer.cpp
        source/SceneLoader.cpp source/Scene.cpp source/SceneObject.cpp
        source/Camera.cpp source/Triangle.cpp source/Sphere.cpp source/Disk.cpp source/Light.cpp)
    target_include_directories(RendererShutdownTests PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include" "${CMAKE_CURRENT_SOURCE_DIR}/minifb/include")
    # MiniFB entry points are supplied by the test so shutdown runs without a GUI.
    target_link_libraries(RendererShutdownTests PRIVATE glm::glm tinyxml2 Threads::Threads)
    add_test(NAME RendererShutdownTests COMMAND RendererShutdownTests
        "${CMAKE_CURRENT_SOURCE_DIR}/scene/CornellBox.xml")
    set_tests_properties(RendererShutdownTests PROPERTIES TIMEOUT 15)

    # Opt in on machines with a working desktop/GPU; regular tests stay headless.
    option(RENDERER_TEST_OPENGL "Register a hidden-window OpenGL smoke test" OFF)
    if(RENDERER_ENABLE_OPENGL AND RENDERER_TEST_OPENGL)
        add_test(NAME OpenGLSmokeTest COMMAND C++Renderer --renderer opengl
            --scene "${CMAKE_CURRENT_SOURCE_DIR}/scene/CornellBox.xml"
            --width 640 --height 360 --hidden --frames 3
            --screenshot "${CMAKE_CURRENT_BINARY_DIR}/opengl-smoke.ppm")
        set_tests_properties(OpenGLSmokeTest PROPERTIES TIMEOUT 30)
        add_test(NAME OpenGLLightsSmokeTest COMMAND C++Renderer --renderer opengl
            --scene "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/OpenGLLights.xml"
            --width 640 --height 360 --hidden --frames 3
            --screenshot "${CMAKE_CURRENT_BINARY_DIR}/opengl-lights.ppm")
        set_tests_properties(OpenGLLightsSmokeTest PROPERTIES TIMEOUT 30)
    endif()
endif()
