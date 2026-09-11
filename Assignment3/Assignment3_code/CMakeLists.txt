cmake_minimum_required(VERSION 3.10)
project(Rasterizer LANGUAGES CXX)

find_package(OpenCV REQUIRED)
find_package(Eigen3 REQUIRED NO_MODULE)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(Rasterizer main.cpp rasterizer.hpp rasterizer.cpp global.hpp Triangle.hpp Triangle.cpp Texture.hpp Texture.cpp Shader.hpp OBJ_Loader.h)
target_include_directories(Rasterizer PRIVATE ${OpenCV_INCLUDE_DIRS})
target_link_libraries(Rasterizer PRIVATE Eigen3::Eigen ${OpenCV_LIBRARIES})

include(CTest)
if(BUILD_TESTING)
    add_test(NAME assignment3_environment COMMAND Rasterizer --check-environment)
    set_tests_properties(assignment3_environment PROPERTIES
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
    if(MINGW)
        # Keep the test process on the same runtime as the compiler.
        get_filename_component(compiler_runtime_dir "${CMAKE_CXX_COMPILER}" DIRECTORY)
        set_tests_properties(assignment3_environment PROPERTIES
            ENVIRONMENT "PATH=${compiler_runtime_dir}")
    endif()
endif()
#target_compile_options(Rasterizer PUBLIC -Wall -Wextra -pedantic)
