cmake_minimum_required(VERSION 3.20)
project(Neural-Network-Circuit-Visualization)

set(CMAKE_CXX_STANDARD 20)

if(EXISTS "${CMAKE_SOURCE_DIR}/SDL3/CMakeLists.txt")
    message(STATUS "SDL3: using local copy")
    add_subdirectory(SDL3)
else()
    message(STATUS "SDL3: fetching from GitHub")
    include(FetchContent)
    FetchContent_Declare(
            SDL3
            GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
            GIT_TAG        release-3.2.18
            GIT_SHALLOW    TRUE
    )
    FetchContent_MakeAvailable(SDL3)
endif()


add_executable(Neural-Network-Circuit-Visualization src/main.cpp
        src/Game.cpp
        src/Game.h
        src/Math.h
        src/Actor.cpp
        src/Actor.h
        src/Transform.cpp
        src/Transform.h
        src/Component.cpp
        src/Component.h
        src/DrawComponent.cpp
        src/DrawComponent.h
        src/Line.cpp
        src/Line.h
        src/NeuralNetwork.cpp
        src/NeuralNetwork.h
        src/Matrix.cpp
        src/Matrix.h
        src/DynamicMatrix.cpp
        src/DynamicMatrix.h
        src/NeuralNetworkActor.cpp
        src/NeuralNetworkActor.h
)

target_link_libraries(Neural-Network-Circuit-Visualization PRIVATE SDL3::SDL3)

if(EMSCRIPTEN)
    set_target_properties(Neural-Network-Circuit-Visualization PROPERTIES
            SUFFIX ".html"
            OUTPUT_NAME "index"
    )
    target_link_options(Neural-Network-Circuit-Visualization PRIVATE
            --shell-file ${CMAKE_SOURCE_DIR}/web/shell.html
            -sALLOW_MEMORY_GROWTH=1
            -sGL_ENABLE_GET_PROC_ADDRESS=1
    )
endif()
