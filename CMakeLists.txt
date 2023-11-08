cmake_minimum_required(VERSION 3.0)
project(rayrm VERSION 0.1 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
add_executable(${PROJECT_NAME} main.cpp)

# 当使用MSVC编译器时，设置/ENTRY:mainCRTStartup链接选项
if(MSVC)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        LINK_FLAGS "/ENTRY:mainCRTStartup"
    )
endif()