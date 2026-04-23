cmake_minimum_required(VERSION 3.17)

project(OpenCV_Project)

find_package(OpenCV REQUIRED)

include_directories(${OpenCV_INCLUDE_DIRS})

add_executable(basic_preprocessing src/basic_preprocessing.cpp)
add_executable(color_detection src/color_detection.cpp)
add_executable(shape_detection src/shape_detection.cpp)
add_executable(armor_detection src/armor_detection.cpp)


target_link_libraries(basic_preprocessing ${OpenCV_LIBS})
target_link_libraries(color_detection ${OpenCV_LIBS})
target_link_libraries(shape_detection ${OpenCV_LIBS})
target_link_libraries(armor_detection ${OpenCV_LIBS})

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)