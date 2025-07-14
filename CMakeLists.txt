cmake_minimum_required(VERSION 3.16)
project(tcp_calculator LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(calculator_lib
    src/server/calculator.cpp
)

target_include_directories(calculator_lib PUBLIC include)

target_compile_options(calculator_lib PRIVATE
    -Wall -Wextra -Wpedantic
)

add_executable(example example/example.cpp)
target_link_libraries(example PRIVATE calculator_lib)
