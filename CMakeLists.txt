cmake_minimum_required(VERSION 3.24)
project(pirate)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_options(-g -O0)

file(GLOB_RECURSE PIRATE_SRC "src/*.cc")

add_library(pirate SHARED ${PIRATE_SRC})

target_include_directories(
    ${PROJECT_NAME}
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(${PROJECT_NAME}
    PUBLIC 
    z minizip
)
