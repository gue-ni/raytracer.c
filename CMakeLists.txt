cmake_minimum_required(VERSION 3.23)
project(ray C)

set(CMAKE_C_STANDARD 99)

add_executable(raytracer main.c)
target_link_libraries(raytracer PRIVATE m)
