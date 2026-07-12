cmake_minimum_required(VERSION 3.10.0)
project(cpersist VERSION 0.1.0 LANGUAGES C CXX)
option(CPERSIST_BUILD_TESTS "Build CPersist tests" ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(cpersist src/includes/cpersist.h src/cpersist.cpp src/includes/error_handler.h src/error_handler.cpp src/includes/serializer.h)


if(CPERSIST_BUILD_TESTS)
    include(CTest)
    enable_testing()

    target_compile_features(cpersist PRIVATE cxx_std_20)

    add_executable(cpersist_test tests/test.cpp)    
    add_test(NAME RunCPersistTests COMMAND cpersist_test)
    target_link_libraries(cpersist_test PRIVATE cpersist)
endif()
    
    
    
target_include_directories(cpersist
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src/includes
)
