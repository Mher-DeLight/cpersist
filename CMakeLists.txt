cmake_minimum_required(VERSION 3.16.0)
project(cpersist VERSION 0.1.0 LANGUAGES C CXX)
option(CPERSIST_BUILD_TESTS "Build CPersist tests" OFF)

add_library(cpersist STATIC src/cpersist.cpp src/error_handler.cpp src/aes.cpp)

target_compile_features(cpersist PUBLIC cxx_std_20)

set_target_properties(cpersist PROPERTIES
    CXX_EXTENSIONS OFF
)

find_package(OpenSSL REQUIRED)

target_link_libraries(cpersist
    PUBLIC
        OpenSSL::Crypto
)


if(CPERSIST_BUILD_TESTS)
    include(CTest)
    enable_testing()    

    add_executable(cpersist_test tests/test.cpp)    
    add_test(NAME RunCPersistTests COMMAND cpersist_test)
    target_link_libraries(cpersist_test PRIVATE cpersist)
endif()
    
    
    
target_include_directories(cpersist
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src/includes
)
 