cmake_minimum_required(VERSION 3.20)
project(Neural-Network-Circuit-Visualization)

set(CMAKE_CXX_STANDARD 20)

add_subdirectory(SDL3)
add_subdirectory(SDL3_image)


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
)

target_link_libraries(Neural-Network-Circuit-Visualization PRIVATE SDL3::SDL3 SDL3_image::SDL3_image)

# Copy assets next to the executable so relative paths work everywhere
add_custom_command(TARGET Neural-Network-Circuit-Visualization POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/src/Assets
        $<TARGET_FILE_DIR:Neural-Network-Circuit-Visualization>/Assets
)

if(EMSCRIPTEN)
    set_target_properties(Neural-Network-Circuit-Visualization PROPERTIES SUFFIX ".html")
    target_link_options(Neural-Network-Circuit-Visualization PRIVATE
            --preload-file ${CMAKE_SOURCE_DIR}/src/Assets@/Assets
            --shell-file ${CMAKE_SOURCE_DIR}/web/shell.html
            -sALLOW_MEMORY_GROWTH=1
            -sGL_ENABLE_GET_PROC_ADDRESS=1
    )
endif()
