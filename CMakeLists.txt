cmake_minimum_required(VERSION 3.16)

project(DroneOS
    VERSION 1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(DroneOS

    src/main.cpp

    src/Kernel.cpp
    src/Kernel.h

    src/Scheduler.cpp
    src/Scheduler.h

    src/ResourceManager.cpp
    src/ResourceManager.h

    src/Drone.cpp
    src/Drone.h

    src/Database.cpp
    src/Database.h

    src/FaultManager.cpp
    src/FaultManager.h

    src/ApiServer.cpp
    src/ApiServer.h
)

target_include_directories(
    DroneOS PRIVATE
    C:/msys64/ucrt64/include
)

target_link_directories(
    DroneOS PRIVATE
    C:/msys64/ucrt64/lib
)

target_link_libraries(
    DroneOS PRIVATE
    sqlite3
    ws2_32
)

if (MSVC)

    target_compile_options(
        DroneOS PRIVATE
        /W4
    )

else()

    target_compile_options(
        DroneOS PRIVATE
        -Wall
        -Wextra
        -Wpedantic
    )

endif()