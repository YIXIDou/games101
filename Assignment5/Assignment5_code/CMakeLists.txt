cmake_minimum_required(VERSION 3.10)
project(RayTracing LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(RayTracing main.cpp Object.hpp Vector.hpp Sphere.hpp global.hpp Triangle.hpp Scene.cpp Scene.hpp Light.hpp Renderer.cpp)
target_compile_features(RayTracing PUBLIC cxx_std_17)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(RayTracing PRIVATE -Wall -Wextra -pedantic -Wshadow -Wreturn-type)

    # MinGW GCC does not ship libubsan in the standard MSYS2 UCRT64 toolchain.
    # Keep the course's undefined-behavior sanitizer on supported Unix builds.
    if(NOT MINGW)
        target_compile_options(RayTracing PRIVATE -fsanitize=undefined)
        target_link_options(RayTracing PRIVATE -fsanitize=undefined)
    endif()
endif()
