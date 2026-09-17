cmake_minimum_required(VERSION 3.20)
project(SmartPointersApp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(lib)
add_subdirectory(external/googletest EXCLUDE_FROM_ALL)

add_executable(${PROJECT_NAME} app/main.cpp benchmark/Benchmark.cpp)

target_link_libraries(${PROJECT_NAME} 
    PRIVATE
        SmartPointersLib
)



enable_testing()

add_executable(SmartPointersTests
        tests/SharedPtrTest.cpp
        tests/UniquePtrTest.cpp
    )

target_compile_features(SmartPointersTests PRIVATE cxx_std_20)

target_link_libraries(SmartPointersTests PRIVATE
        SmartPointersLib
        GTest::gtest_main
    )

add_test(
        NAME SmartPointers
        COMMAND SmartPointersTests
    )