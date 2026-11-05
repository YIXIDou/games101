cmake_minimum_required(VERSION 3.10)
project(RayTracing)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(RayTracing main.cpp Object.hpp Vector.cpp Vector.hpp Sphere.hpp global.hpp Triangle.hpp Scene.cpp
        Scene.hpp Light.hpp AreaLight.hpp BVH.cpp BVH.hpp Bounds3.hpp Ray.hpp Material.hpp Intersection.hpp
        Renderer.cpp Renderer.hpp)

# This checks startup, model loading and BVH construction without calling the
# unfinished intersection/path-tracing functions.
include(CTest)
if(BUILD_TESTING)
    add_test(NAME assignment7_environment
        COMMAND RayTracing --check-environment)
    set_tests_properties(assignment7_environment PROPERTIES
        WORKING_DIRECTORY "$<TARGET_FILE_DIR:RayTracing>"
        TIMEOUT 30)
endif()
