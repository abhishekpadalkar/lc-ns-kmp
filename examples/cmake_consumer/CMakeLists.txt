# Minimal consumer used to smoke-test find_package(lc-ns-kmp) after install.
cmake_minimum_required(VERSION 3.16)
project(lc_ns_kmp_cmake_consumer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(lc-ns-kmp 0.1 REQUIRED)

add_executable(consumer main.cpp)
target_link_libraries(consumer PRIVATE lc-ns-kmp::gmr lc-ns-kmp::lc-ns-kmp)
