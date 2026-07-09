cmake_minimum_required(VERSION 3.10.0)
project(cpersist VERSION 0.1.0 LANGUAGES C CXX)

add_library(cpersist src/includes/cpersist.h src/cpersist.cpp)

include(CTest)
enable_testing()

add_executable(cpersist_test tests/test.cpp)

target_link_libraries(cpersist_test PRIVATE cpersist)
target_include_directories(cpersist
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src/includes
)

add_test(NAME RunCPersistTests COMMAND cpersist_test)