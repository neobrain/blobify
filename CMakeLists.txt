cmake_minimum_required(VERSION 3.12)
project(blobify LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")


# Check if the user provides dependencies using Conan
include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake OPTIONAL RESULT_VARIABLE CONAN_ENABLED)
if(CONAN_ENABLED)
    conan_basic_setup(TARGETS)
endif()


# Build options
option(BLOBIFY_TESTS "Build tests" OFF)


# Find dependencies
find_package(PFR REQUIRED)

if(BLOBIFY_TESTS)
    find_package(Catch2)
    if(NOT Catch2_FOUND)
        message(FATAL_ERROR "Building tests was requested, but Catch2 could not be found")
    endif()
endif()


# Setup core library
add_library(blobify INTERFACE)
target_link_libraries(blobify INTERFACE pfr::pfr)
target_include_directories(blobify INTERFACE
    $<INSTALL_INTERFACE:include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
target_compile_features(blobify INTERFACE cxx_std_17)


# Setup associated sources for static analysis
# These are a bunch of empty source files that include our headers, so that tools will actually parse them
add_library(blobify-associated-sources STATIC src/blobify.cpp)
target_link_libraries(blobify-associated-sources blobify)


# Examples and tests
add_subdirectory(examples)
if (BLOBIFY_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()


# Install targets
install(TARGETS blobify EXPORT blobifyTargets)
install(DIRECTORY include/ DESTINATION include)
install(EXPORT blobifyTargets
    FILE blobifyTargets.cmake
    NAMESPACE blobify::
    DESTINATION lib/cmake/blobify)

# TODO: Add version information via write_basic_package_version_file
