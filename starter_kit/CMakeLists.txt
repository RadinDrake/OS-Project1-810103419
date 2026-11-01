cmake_minimum_required(VERSION 3.12)
project(OS_Structures_Simulation CXX)
enable_testing()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/W4 /WX-)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -O2)
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_executable(test_runner
    test_runner.cpp
    platform/Tracer.cpp
    1_monolithic/MonolithicKernel.cpp
    1_monolithic/MonolithicOSApi.cpp
    2_layered/Layer0_Hardware.cpp
    2_layered/Layer1_Drivers.cpp
    2_layered/Layer2_FileSystem.cpp
    2_layered/Layer3_OSApi.cpp
    3_modular/StandardModules.cpp
    3_modular/ModularKernel.cpp
    3_modular/ModularOSApi.cpp
)

add_test(NAME structures_tests COMMAND test_runner)
