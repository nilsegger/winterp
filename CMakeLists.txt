cmake_minimum_required(VERSION 3.10)

project(winterp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Copy all test binaries to the build directory
file(COPY ${CMAKE_SOURCE_DIR}/test_binaries DESTINATION ${CMAKE_BINARY_DIR})

set(SOURCES
    src/sections.cpp
    src/leb128.cpp
    src/instructions.cpp
    src/runtime.cpp
)

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(
    ${PROJECT_NAME}_test
    tests/leb128.cpp
    tests/test_01.cpp
    tests/test_02.cpp
    tests/test_03.cpp
    tests/test_04.cpp
    tests/test_05.cpp
    tests/test_07.cpp
    tests/test_09.cpp
    ${SOURCES}
 )

target_link_libraries(
    ${PROJECT_NAME}_test
    GTest::gtest_main
)

target_include_directories(
    ${PROJECT_NAME}_test
    PRIVATE
    ${PROJECT_SOURCE_DIR}/include
)

include(GoogleTest)
gtest_discover_tests(
    ${PROJECT_NAME}_test
)
