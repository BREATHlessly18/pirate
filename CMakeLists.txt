cmake_minimum_required(VERSION 3.24)
# CMake 4 dropped compatibility with < 3.5; libarchive 3.7 still declares an older minimum.
if(NOT DEFINED CMAKE_POLICY_VERSION_MINIMUM)
  set(CMAKE_POLICY_VERSION_MINIMUM 3.5 CACHE STRING "" FORCE)
endif()
project(pirate VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

option(PIRATE_BUILD_SHARED "Build pirate as a shared library" ON)
option(PIRATE_BUILD_TEST "Build pirate_test CLI" ON)
option(PIRATE_BUILD_UNIT_TESTS "Build GoogleTest unit tests" ON)
option(PIRATE_ENABLE_COVERAGE "Instrument pirate for coverage (GCC/Clang --coverage)" OFF)
option(PIRATE_COVERAGE_FAIL_UNDER "Fail coverage target if branch % is below this" 80)

include(FetchContent)

# ---------------------------------------------------------------------------
# fmt (used by pirate_test)
# ---------------------------------------------------------------------------
set(FMT_DOC OFF CACHE BOOL "" FORCE)
set(FMT_TEST OFF CACHE BOOL "" FORCE)
set(FMT_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    fmt
    GIT_REPOSITORY https://github.com/fmtlib/fmt.git
    GIT_TAG        11.2.0
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(fmt)

# ---------------------------------------------------------------------------
# zlib: UNIX uses the system package. MSVC has no zlib, so build+install it
# at configure time into the build tree, then find_package(ZLIB) so
# libarchive enables deflate (HAVE_LIBZ). Listing works without zlib;
# extracting compressed members does not.
# ---------------------------------------------------------------------------
if(WIN32)
  set(ZLIB_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
  FetchContent_Declare(
      zlib
      GIT_REPOSITORY https://github.com/madler/zlib.git
      GIT_TAG        v1.3.1
      GIT_SHALLOW    TRUE
  )
  FetchContent_GetProperties(zlib)
  if(NOT zlib_POPULATED)
    FetchContent_Populate(zlib)
  endif()

  set(_pirate_zlib_host "${CMAKE_BINARY_DIR}/_zlib_host")
  set(_pirate_zlib_inst "${CMAKE_BINARY_DIR}/_zlib_install")
  if(NOT EXISTS "${_pirate_zlib_inst}/include/zlib.h")
    message(STATUS "Configuring and installing zlib for MSVC...")
    set(_pirate_zlib_gen -S "${zlib_SOURCE_DIR}" -B "${_pirate_zlib_host}"
        -G "${CMAKE_GENERATOR}"
        -DCMAKE_INSTALL_PREFIX=${_pirate_zlib_inst}
        -DCMAKE_POLICY_VERSION_MINIMUM=3.5
        -DCMAKE_POSITION_INDEPENDENT_CODE=ON)
    if(CMAKE_GENERATOR_PLATFORM)
      list(APPEND _pirate_zlib_gen -A "${CMAKE_GENERATOR_PLATFORM}")
    endif()
    execute_process(
        COMMAND ${CMAKE_COMMAND} ${_pirate_zlib_gen}
        RESULT_VARIABLE _pirate_zlib_cfg
        COMMAND_ERROR_IS_FATAL ANY)
    execute_process(
        COMMAND ${CMAKE_COMMAND} --build "${_pirate_zlib_host}"
                --config Release --target install
        RESULT_VARIABLE _pirate_zlib_bld
        COMMAND_ERROR_IS_FATAL ANY)
  endif()

  set(ZLIB_ROOT "${_pirate_zlib_inst}" CACHE PATH "" FORCE)
  list(PREPEND CMAKE_PREFIX_PATH "${_pirate_zlib_inst}")
  unset(ZLIB_LIBRARY CACHE)
  unset(ZLIB_LIBRARIES CACHE)
  unset(ZLIB_INCLUDE_DIR CACHE)
  unset(ZLIB_FOUND CACHE)
endif()

find_package(ZLIB REQUIRED)

# ---------------------------------------------------------------------------
# libarchive (static). ZIP now; tar/gz/zst later.
# BUILD_SHARED_LIBS is forced OFF only for this dependency.
# ---------------------------------------------------------------------------
set(_pirate_saved_bsl "${BUILD_SHARED_LIBS}")
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

set(ENABLE_TAR OFF CACHE BOOL "" FORCE)
set(ENABLE_TAR_SHARED OFF CACHE BOOL "" FORCE)
set(ENABLE_CPIO OFF CACHE BOOL "" FORCE)
set(ENABLE_CAT OFF CACHE BOOL "" FORCE)
set(ENABLE_UNZIP OFF CACHE BOOL "" FORCE)
set(ENABLE_TEST OFF CACHE BOOL "" FORCE)
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
set(ENABLE_COVERAGE OFF CACHE BOOL "" FORCE)
set(ENABLE_LIBXML2 OFF CACHE BOOL "" FORCE)
set(ENABLE_EXPAT OFF CACHE BOOL "" FORCE)
set(ENABLE_PCREPOSIX OFF CACHE BOOL "" FORCE)
set(ENABLE_PCRE2POSIX OFF CACHE BOOL "" FORCE)
set(ENABLE_CNG OFF CACHE BOOL "" FORCE)
set(ENABLE_BZip2 OFF CACHE BOOL "" FORCE)
set(ENABLE_LZMA OFF CACHE BOOL "" FORCE)
set(ENABLE_LZO OFF CACHE BOOL "" FORCE)
set(ENABLE_LZ4 OFF CACHE BOOL "" FORCE)
set(ENABLE_ZSTD OFF CACHE BOOL "" FORCE)
set(ENABLE_OPENSSL OFF CACHE BOOL "" FORCE)
set(ENABLE_MBEDTLS OFF CACHE BOOL "" FORCE)
set(ENABLE_NETTLE OFF CACHE BOOL "" FORCE)
set(ENABLE_LIBB2 OFF CACHE BOOL "" FORCE)
set(ENABLE_ACL OFF CACHE BOOL "" FORCE)
set(ENABLE_ICONV OFF CACHE BOOL "" FORCE)
set(ENABLE_XATTR OFF CACHE BOOL "" FORCE)
set(ENABLE_WERROR OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    libarchive
    GIT_REPOSITORY https://github.com/libarchive/libarchive.git
    GIT_TAG        v3.7.7
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(libarchive)

set(BUILD_SHARED_LIBS "${_pirate_saved_bsl}" CACHE BOOL "" FORCE)

if(TARGET archive_static)
  set(PIRATE_ARCHIVE_LIB archive_static)
elseif(TARGET archive)
  set(PIRATE_ARCHIVE_LIB archive)
else()
  message(FATAL_ERROR "libarchive did not create archive or archive_static")
endif()

# ---------------------------------------------------------------------------
# pirate
# ---------------------------------------------------------------------------
# Explicit sources (do not GLOB leftover experimental files).
set(PIRATE_SOURCES
    src/pirate/archive_iterator.cc
    src/pirate/zip_archive.cc
)

if(PIRATE_BUILD_SHARED)
  add_library(pirate SHARED ${PIRATE_SOURCES})
else()
  add_library(pirate STATIC ${PIRATE_SOURCES})
endif()

add_library(pirate::pirate ALIAS pirate)

target_include_directories(pirate
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
)

target_link_libraries(pirate PRIVATE ${PIRATE_ARCHIVE_LIB} ZLIB::ZLIB)
if(MSVC)
  target_compile_definitions(pirate PRIVATE LIBARCHIVE_STATIC)
endif()
if(WIN32 AND DEFINED _pirate_zlib_inst)
  add_custom_command(
      TARGET pirate
      POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
              "${_pirate_zlib_inst}/bin/zlib.dll"
              $<TARGET_FILE_DIR:pirate>
      COMMENT "Copy zlib.dll next to pirate.dll")
endif()

if(UNIX)
  find_package(Threads REQUIRED)
  target_link_libraries(pirate PRIVATE Threads::Threads ${CMAKE_DL_LIBS})
endif()

# libstdc++fs was separate before GCC 9
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9.0)
  target_link_libraries(pirate PRIVATE stdc++fs)
endif()

if(MSVC)
  target_compile_options(pirate PRIVATE /W4 /utf-8)
  target_compile_definitions(pirate PRIVATE _CRT_SECURE_NO_WARNINGS)
else()
  target_compile_options(pirate PRIVATE -Wall -Wextra)
endif()

if(PIRATE_ENABLE_COVERAGE)
  if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(pirate PRIVATE --coverage -O0 -g)
    target_link_options(pirate PRIVATE --coverage)
  endif()
endif()

# ---------------------------------------------------------------------------
# test
# ---------------------------------------------------------------------------
if(PIRATE_BUILD_TEST)
  add_executable(pirate_test test/main.cc)
  target_link_libraries(pirate_test PRIVATE pirate::pirate fmt::fmt)
  if(MSVC)
    target_compile_options(pirate_test PRIVATE /W4 /utf-8)
  else()
    target_compile_options(pirate_test PRIVATE -Wall -Wextra)
  endif()
endif()

if(PIRATE_BUILD_UNIT_TESTS)
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
  set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
  set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
  FetchContent_Declare(
      googletest
      GIT_REPOSITORY https://github.com/google/googletest.git
      GIT_TAG        v1.15.2
      GIT_SHALLOW    TRUE
  )
  FetchContent_MakeAvailable(googletest)

  add_executable(pirate_unit_tests
      test/unit/path_test.cc
      test/unit/archive_test.cc
  )
  target_include_directories(pirate_unit_tests PRIVATE test/unit)
  target_compile_definitions(pirate_unit_tests PRIVATE PIRATE_UNIT_TEST)
  target_link_libraries(pirate_unit_tests PRIVATE pirate::pirate GTest::gtest_main ZLIB::ZLIB)
  if(MSVC)
    target_compile_options(pirate_unit_tests PRIVATE /W4 /utf-8)
    target_compile_definitions(pirate_unit_tests PRIVATE _CRT_SECURE_NO_WARNINGS)
  else()
    target_compile_options(pirate_unit_tests PRIVATE -Wall -Wextra)
  endif()
  if(WIN32 AND DEFINED _pirate_zlib_inst)
    add_custom_command(
        TARGET pirate_unit_tests
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${_pirate_zlib_inst}/bin/zlib.dll"
                $<TARGET_FILE_DIR:pirate_unit_tests>
        COMMENT "Copy zlib.dll next to pirate_unit_tests")
  endif()

  include(GoogleTest)
  enable_testing()
  gtest_discover_tests(pirate_unit_tests
      DISCOVERY_MODE PRE_TEST
      WORKING_DIRECTORY $<TARGET_FILE_DIR:pirate_unit_tests>
  )

  add_custom_target(pirate_coverage
      COMMAND ${CMAKE_COMMAND}
              -DPIRATE_SOURCE_DIR=${CMAKE_SOURCE_DIR}
              -DPIRATE_BINARY_DIR=${CMAKE_BINARY_DIR}
              -DPIRATE_UNIT_TEST_EXE=$<TARGET_FILE:pirate_unit_tests>
              -DPIRATE_COVERAGE_FAIL_UNDER=${PIRATE_COVERAGE_FAIL_UNDER}
              -P ${CMAKE_SOURCE_DIR}/cmake/check_coverage.cmake
      DEPENDS pirate_unit_tests
      WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
      COMMENT "Run unit tests and enforce branch coverage >= ${PIRATE_COVERAGE_FAIL_UNDER}%"
  )
endif()
