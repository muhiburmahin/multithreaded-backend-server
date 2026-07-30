cmake_minimum_required(VERSION 3.10)
project(multithreaded_backend_server LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(ENABLE_TSAN "Enable ThreadSanitizer" OFF)

find_package(Threads REQUIRED)

add_executable(server
    src/main.cpp
    src/Server.cpp
    src/ThreadPool.cpp
    src/RateLimiter.cpp
    src/HttpRequest.cpp
    src/HttpResponse.cpp
    src/Router.cpp
    src/Logger.cpp
)

target_include_directories(server PRIVATE include)
target_link_libraries(server PRIVATE Threads::Threads)

if(ENABLE_TSAN)
    target_compile_options(server PRIVATE -fsanitize=thread -g)
    target_link_options(server PRIVATE -fsanitize=thread)
endif()
