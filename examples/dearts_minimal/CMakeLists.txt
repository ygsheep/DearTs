cmake_minimum_required(VERSION 3.20)
project(DeartsMinimal LANGUAGES CXX)

# 通过 find_package(dearts) 使用安装的 DearTs 框架
find_package(dearts 1.0 REQUIRED)

add_executable(dearts_minimal main.cpp)

target_link_libraries(dearts_minimal PRIVATE DearTs::dearts)
