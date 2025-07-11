cmake_minimum_required(VERSION 3.16)

project(dz_bench VERSION 0.1 LANGUAGES CXX)

find_package(benchmark CONFIG REQUIRED)
#find_package(Qt6 REQUIRED COMPONENTS Gui) # jpg/png encoding

add_executable(${PROJECT_NAME}
    ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE dz_openslide
    PRIVATE dz_qupath
    PRIVATE dz_slideio
    PRIVATE benchmark::benchmark
    #PRIVATE Qt${QT_VERSION_MAJOR}::Gui
)
