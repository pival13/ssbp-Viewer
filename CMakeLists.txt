cmake_minimum_required(VERSION 3.0.0)

set(PROJECT ssbpPlayer)
project(${PROJECT})

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)
find_package(ImageMagick COMPONENTS Magick++ MagickCore MagickWand REQUIRED)
string(REPLACE "_RL_" "_DB_" ImageMagick_LIBRARIES_DEBUG "${ImageMagick_LIBRARIES}")

add_compile_options(
    -wd4251 # needs to have dll-interface
    -wd4275 # non dll-interface class used as base for dll-interface class
)

set(SOURCES
    src/glad.c
    src/Screenshot.cpp
    src/SsbpResource.cpp
    src/SsbpReader.cpp
    src/SsbpPlayer.cpp
    src/SsbpViewer.cpp
)

add_executable(${PROJECT}
    ${SOURCES}
    src/main.cpp
)

target_include_directories(${PROJECT} PRIVATE
    ${PROJECT_SOURCE_DIR}
    include
    ${ImageMagick_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT}
    opengl32
    glfw
    glm::glm

    debug
        "${ImageMagick_LIBRARIES_DEBUG}"
    optimized
        "${ImageMagick_LIBRARIES}"
)

add_custom_command(
    TARGET ${PROJECT} POST_BUILD
    #COMMAND $<IF:$<PLATFORM_ID:Windows>,dir,ls> "${ImageMagick_INCLUDE_DIRS}/.."
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:${PROJECT}>" "${PROJECT_SOURCE_DIR}/bin"
)