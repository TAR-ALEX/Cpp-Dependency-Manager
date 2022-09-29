cmake_minimum_required(VERSION 3.16)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# set(CMAKE_VERBOSE_MAKEFILE ON)

# if (MSVC_IDE)
#     # hack to get around the "Debug" and "Release" directories cmake tries to add on Windows
#     set_target_properties (${NAME} PROPERTIES PREFIX "../")
# endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})
project(git-vendor)

include_directories(
    "./vendor/include/"
    "./include/"
    "./src"
    "./vendor/src"
) 

file(GLOB_RECURSE APP_SOURCES CONFIGURE_DEPENDS "./src/*.cpp" "./vendor/src/*.cpp")

add_executable(${PROJECT_NAME} ${APP_SOURCES})

target_link_libraries(${PROJECT_NAME} PRIVATE -lstdc++fs -lssl -lcrypto -llzma -lz -lbz2 -lpthread -lncurses)


