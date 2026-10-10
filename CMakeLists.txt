cmake_minimum_required(VERSION 3.16)
project(multithreaded_http_server LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(http_server
    src/main.cpp
    src/http_server.cpp
    src/http_parser.cpp
    src/http_response.cpp
    src/router.cpp
    src/thread_pool.cpp
    src/logger.cpp
)

target_include_directories(http_server PRIVATE include)
target_compile_options(http_server PRIVATE -Wall -Wextra -Wpedantic)
target_link_libraries(http_server PRIVATE pthread)
