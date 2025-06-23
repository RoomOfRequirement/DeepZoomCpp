cmake_minimum_required(VERSION 3.16)

project(dz_qupath VERSION 0.1 LANGUAGES CXX)

find_package(Java REQUIRED)
find_package(JNI REQUIRED)

include(UseJava)
set(JAVA_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/java)
file(MAKE_DIRECTORY ${JAVA_OUTPUT_DIR})
file(GLOB JAVA_SOURCES LIST_DIRECTORIES false CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/*.java
)
file(GLOB JARS LIST_DIRECTORIES false CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/qupath_jars/*.jar
)
add_jar(${PROJECT_NAME}_qpwrapper
    SOURCES ${JAVA_SOURCES}
    OUTPUT_DIR ${JAVA_OUTPUT_DIR}
    INCLUDE_JARS  ${JARS}
    ENTRY_POINT ${PROJECT_NAME}
)

add_library(${PROJECT_NAME}_qpreader
    STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/jvmwrapper.cpp ${CMAKE_CURRENT_SOURCE_DIR}/jvmwrapper.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/reader.cpp ${CMAKE_CURRENT_SOURCE_DIR}/reader.hpp
)
add_dependencies(${PROJECT_NAME}_qpreader
    ${PROJECT_NAME}
)
target_include_directories(${PROJECT_NAME}_qpreader
    PUBLIC ${JNI_INCLUDE_DIRS}
)
target_link_libraries(${PROJECT_NAME}_qpreader
    PUBLIC ${JNI_LIBRARIES}
)

add_executable(${PROJECT_NAME}_qpreader_test
    ${CMAKE_CURRENT_SOURCE_DIR}/reader_test.cpp
)
target_link_libraries(${PROJECT_NAME}_qpreader_test
    PRIVATE ${PROJECT_NAME}_qpreader
)

add_library(${PROJECT_NAME}
    STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/deepzoom.cpp ${CMAKE_CURRENT_SOURCE_DIR}/deepzoom.hpp
)
add_dependencies(${PROJECT_NAME}
    ${PROJECT_NAME}_qpwrapper
)
target_link_libraries(${PROJECT_NAME}
    PUBLIC ${PROJECT_NAME}_qpreader
)

add_executable(${PROJECT_NAME}_test
    ${CMAKE_CURRENT_SOURCE_DIR}/deepzoom_test.cpp
)
target_link_libraries(${PROJECT_NAME}_test
    PRIVATE ${PROJECT_NAME}
)
