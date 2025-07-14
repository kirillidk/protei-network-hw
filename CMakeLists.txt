cmake_minimum_required(VERSION 3.16)
project(tcp_calculator LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(calculator_lib
    src/server/calculator.cpp
    src/server/tcp_server.cpp
)

target_include_directories(calculator_lib PUBLIC include)

target_compile_options(calculator_lib PRIVATE
    -Wall -Wextra -Wpedantic
)

add_library(client_lib
    src/client/tcp_client.cpp
)

target_include_directories(client_lib PUBLIC include)
target_link_libraries(client_lib PRIVATE calculator_lib)

target_compile_options(client_lib PRIVATE
    -Wall -Wextra -Wpedantic
)

add_executable(server src/server/main.cpp)
target_link_libraries(server PRIVATE calculator_lib)

add_executable(client src/client/main.cpp)
target_link_libraries(client PRIVATE client_lib)