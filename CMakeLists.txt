cmake_minimum_required(VERSION 3.24)
project(pirate)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


set(MINIZIP_DIR ${CMAKE_SOURCE_DIR}/thirdparty/minizip)

include(FetchContent)

message(STATUS "Minizip not found, fetching from GitHub...")

# FetchContent_Declare(
#     minizip
#     GIT_REPOSITORY https://github.com/domoticz/minizip.git
#     GIT_TAG        master
#     SOURCE_DIR     ${MINIZIP_DIR}
# )
# FetchContent_Populate(minizip)

message(STATUS "Fmt not found, fetching from GitHub...")
FetchContent_Declare(
    fmt
    GIT_REPOSITORY https://github.com/fmtlib/fmt.git
    GIT_TAG 11.2.0
)
FetchContent_MakeAvailable(fmt)

add_compile_options(-g -O0)

file(GLOB_RECURSE PIRATE_SRC "src/*.cc")

add_subdirectory(thirdparty/minizip)

add_library(pirate SHARED ${PIRATE_SRC})

target_include_directories(${PROJECT_NAME}
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/minizip
)

target_link_libraries(${PROJECT_NAME}
    PUBLIC 
    z minizip
    fmt::fmt
)
