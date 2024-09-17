cmake_minimum_required(VERSION 3.10)

project(algorithm_visualization VERSION 1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(SFML 2.5 COMPONENTS graphics window system REQUIRED)

file(GLOB SOURCES "src/*.cpp")

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/../bin)

foreach(SOURCE ${SOURCES})
    get_filename_component(EXECUTABLE_NAME ${SOURCE} NAME_WE)
    
    add_executable(${EXECUTABLE_NAME} ${SOURCE})
    
    # Link SFML libraries
    target_link_libraries(${EXECUTABLE_NAME} sfml-graphics sfml-window sfml-system)
    
    # Include directories for SFML
    include_directories(${SFML_INCLUDE_DIRS})
endforeach()
