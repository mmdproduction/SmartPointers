cmake_minimum_required(VERSION 3.20)
project(SmartPointersApp LANGUAGE CXX)

set(CXX_STANDARD 20)

add_subdirectory(lib)

add_executable(app/main.cpp)

target_link_libraries(${PROJECT_NAME} 
    PRIVATE
        SmartPointersLib
)

add_test()