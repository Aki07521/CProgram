cmake_minimum_required(VERSION 3.20)

project(RestaurantQt LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/dist)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

add_executable(restaurant_qt
    qt_main.cpp
    restaurant_backend.cpp
    restaurant_backend.hpp
)

target_link_libraries(restaurant_qt PRIVATE Qt6::Widgets)
