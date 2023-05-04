cmake_minimum_required(VERSION 3.16)
project(git-vendor) # this has to be first, otherwise bad things happen

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# set(CMAKE_VERBOSE_MAKEFILE ON)

option(BUILD_LIBS_FROM_SCRATCH "BUILD LIBRARIES FROM SCRATCH, almost no dependencies link wise" ON)

set(LIB_SEARCH_DIR "${CMAKE_SOURCE_DIR}/vendor/lib/")

if(MSVC OR BUILD_LIBS_FROM_SCRATCH OR APPLE)
    add_subdirectory(vendor/zlib)
    add_subdirectory(vendor/bzip2)
    add_subdirectory(vendor/liblzma)
    add_subdirectory(vendor/openssl-cmake)
    add_subdirectory(vendor/zstd/build/cmake)
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR})
# SET( CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG "${CMAKE_SOURCE_DIR}")
# SET( CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}")
# SET( CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG "${CMAKE_SOURCE_DIR}")
# SET( CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}")

include_directories(
    ${zlib_BINARY_DIR}
    ${bzip2_BINARY_DIR}
    ${xz_BINARY_DIR}
    ${openssl_BINARY_DIR}
    ${ZSTD_BINARY_DIR}

    ${zlib_SOURCE_DIR}
    ${bzip2_SOURCE_DIR}
    ${xz_SOURCE_DIR}/src/liblzma/api/
    ${openssl_SOURCE_DIR}
    ${ZSTD_SOURCE_DIR}
    "./vendor/zstd/lib/"

    "./vendor/include/"
    "./include/"
    "./src"
    "./vendor/src"
)


file(GLOB_RECURSE APP_SOURCES CONFIGURE_DEPENDS "./src/*" "./include/*" "./vendor/src/*" "./vendor/include/*")

add_executable(${PROJECT_NAME} ${APP_SOURCES})


if(APPLE)
    target_link_libraries(${PROJECT_NAME} PRIVATE "-framework CoreFoundation" "-framework Security" -lpthread zlibstatic BZip2::BZip2 crypto ssl liblzma libzstd_static)
elseif(MSVC OR BUILD_LIBS_FROM_SCRATCH)
    target_link_libraries(${PROJECT_NAME} PRIVATE -lstdc++fs -lpthread zlibstatic BZip2::BZip2 crypto ssl liblzma libzstd_static)
else()
    target_link_libraries(${PROJECT_NAME} PRIVATE -lstdc++fs -lssl -lcrypto -llzma -lz -lzstd -lbz2 -lpthread)    
endif()


