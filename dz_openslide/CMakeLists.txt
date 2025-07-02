cmake_minimum_required(VERSION 3.16)

project(dz_openslide VERSION 0.1 LANGUAGES CXX)

find_library(openslide
    NAMES openslide
    PATHS ENV OPENSLIDE_LIB_PATH
    NO_CACHE
    REQUIRED
)
if (${openslide} STREQUAL "openslide-NOTFOUND")
    message(FATAL_ERROR "required openslide library but not found!")
else()
    message(STATUS "openslide library found in ${openslide}")
    get_filename_component(openslide_lib_dir ${openslide} PATH)
    get_filename_component(openslide_dir ${openslide_lib_dir} PATH)
    set(openslide_INCLUDE_DIRS ${openslide_dir}/include/openslide)
    message(STATUS "openslide include dirs: ${openslide_INCLUDE_DIRS}")
endif()

find_package(JPEG REQUIRED)
find_package(PNG REQUIRED)

#find_package(Qt6 REQUIRED COMPONENTS Gui) # jpg/png encoding

add_library(${PROJECT_NAME}
    ${CMAKE_CURRENT_SOURCE_DIR}/deepzoom.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/deepzoom.hpp
)
target_include_directories(${PROJECT_NAME} PUBLIC ${openslide_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME}
    PUBLIC ${openslide}
    PUBLIC JPEG::JPEG
    PUBLIC PNG::PNG
)

add_executable(${PROJECT_NAME}_test
    ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/deepzoom.cpp
)
target_link_libraries(${PROJECT_NAME}_test
    PRIVATE ${PROJECT_NAME}
    #PRIVATE Qt${QT_VERSION_MAJOR}::Gui
)

add_custom_command(TARGET ${PROJECT_NAME}_test POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${openslide_dir}/bin/libopenslide-1.dll"
        $<TARGET_FILE_DIR:${PROJECT_NAME}_test>
)
