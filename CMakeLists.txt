cmake_minimum_required(VERSION 3.16)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# set(CMAKE_VERBOSE_MAKEFILE ON)

# SET( CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_SOURCE_DIR}")
# SET( CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}")
# SET( CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG "${CMAKE_SOURCE_DIR}")
# SET( CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}")
# SET( CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG "${CMAKE_SOURCE_DIR}")
# SET( CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}")

option(BUILD_LIBS_FROM_SCRATCH "BUILD LIBRARIES FROM SCRATCH, no dependencies link wise" OFF)

set(LIB_SEARCH_DIR "${CMAKE_SOURCE_DIR}/vendor/lib/")

if(BUILD_LIBS_FROM_SCRATCH)
    add_subdirectory(vendor/zlib)
    add_subdirectory(vendor/bzip2)
    add_subdirectory(vendor/liblzma)
    add_subdirectory(vendor/openssl-cmake)
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})
project(git-vendor)

include_directories(
    ${zlib_BINARY_DIR}
    ${bzip2_BINARY_DIR}
    ${xz_BINARY_DIR}
    ${openssl_BINARY_DIR}

    ${zlib_SOURCE_DIR}
    ${bzip2_SOURCE_DIR}
    ${xz_SOURCE_DIR}/src/liblzma/api/
    ${openssl_SOURCE_DIR}

    "./vendor/include/"
    "./include/"
    "./src"
    "./vendor/src"
)

file(GLOB_RECURSE APP_SOURCES CONFIGURE_DEPENDS "./src/*.cpp" "./vendor/src/*.cpp")

add_executable(${PROJECT_NAME} ${APP_SOURCES})
# target_link_libraries(${PROJECT_NAME} PRIVATE -lstdc++fs -llzma -lz -lbz2 -lpthread)
if(BUILD_LIBS_FROM_SCRATCH)
    target_link_libraries(${PROJECT_NAME} PRIVATE zlibstatic BZip2::BZip2 crypto ssl liblzma)
else()
    target_link_libraries(${PROJECT_NAME} PRIVATE -lstdc++fs -lssl -lcrypto -llzma -lz -lbz2 -lpthread)    
endif()


