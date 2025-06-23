cmake_minimum_required(VERSION 3.16)

project(DeepZoomCpp VERSION 0.1 LANGUAGES CXX)

set(CMAKE_SUPPRESS_DEVELOPER_WARNINGS ON CACHE INTERNAL "" FORCE)  # -Wno-dev

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(dz_openslide)
add_subdirectory(dz_qupath)
