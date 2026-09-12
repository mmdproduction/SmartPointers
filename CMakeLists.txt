cmake_minimum_required(VERSION 3.20)
project(SmartPointersApp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(lib)

add_executable(${PROJECT_NAME} app/main.cpp)

target_link_libraries(${PROJECT_NAME} 
    PRIVATE
        SmartPointersLib
)