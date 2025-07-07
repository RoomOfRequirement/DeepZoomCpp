cmake_minimum_required(VERSION 3.16)

project(dz_slideio VERSION 0.1 LANGUAGES CXX)

find_library(slideio
    NAMES slideio
    PATHS ENV SLIDEIO_LIB_PATH
    NO_CACHE
    REQUIRED
)
if (${slideio} STREQUAL "slideio-NOTFOUND")
    message(FATAL_ERROR "required slideio library but not found!")
else()
    message(STATUS "slideio library found in ${slideio}")
    get_filename_component(slideio_lib_dir ${slideio} PATH)
    get_filename_component(slideio_dir ${slideio_lib_dir} PATH)
    set(slideio_INCLUDE_DIRS ${slideio_dir}/include/)
    message(STATUS "slideio include dirs: ${slideio_INCLUDE_DIRS}")
    set(slideio_Libraries ${slideio_lib_dir}/*)
endif()

find_package(JPEG REQUIRED)

add_executable(${PROJECT_NAME}
    ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
)
target_include_directories(${PROJECT_NAME} PUBLIC ${slideio_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME}
    PUBLIC ${slideio_Libraries}
    PUBLIC JPEG::JPEG
)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
        "${slideio_dir}/bin/"
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/
)
