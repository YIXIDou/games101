cmake_minimum_required(VERSION 3.10)
project(Rasterizer LANGUAGES CXX)

find_package(OpenCV REQUIRED)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(Rasterizer main.cpp rasterizer.hpp rasterizer.cpp global.hpp Triangle.hpp Triangle.cpp)
target_include_directories(Rasterizer PRIVATE ${OpenCV_INCLUDE_DIRS})

if(EXISTS "/usr/local/include")
    target_include_directories(Rasterizer PRIVATE /usr/local/include)
endif()

target_link_libraries(Rasterizer ${OpenCV_LIBRARIES})
