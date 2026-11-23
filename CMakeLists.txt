cmake_minimum_required(VERSION 3.12)
project(DistributedStorage LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Threads REQUIRED)

set(COMMON_SOURCES
    src/Config.cpp
    src/Schema.cpp
    src/Distributor.cpp
    src/InMemoryStorage.cpp
    src/TcpTransport.cpp
    src/TcpNodeServer.cpp
    src/Node.cpp
    src/DataLoader.cpp
)

add_executable(node_server
    src/node_server_main.cpp
    ${COMMON_SOURCES}
)

add_executable(cluster_verify
    src/cluster_verify.cpp
    src/Config.cpp
    src/Distributor.cpp
    src/TcpTransport.cpp
)

foreach(target node_server cluster_verify)
    target_include_directories(${target} PRIVATE src)
    target_link_libraries(${target} PRIVATE Threads::Threads)
    target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
endforeach()
