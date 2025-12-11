cmake_minimum_required(VERSION 3.10.0)

set(PROJECT SsbpPlayer)
project(${PROJECT})

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenGL COMPONENTS OpenGL REQUIRED)
find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)
find_package(ImageMagick COMPONENTS Magick++ MagickCore MagickWand REQUIRED)

set(SOURCES
    src/glad/glad.c
    src/main.cpp

    src/SsbpReader.cpp
    src/SsbpPlayer.cpp

    src/tools/GLTexture.cpp
    src/tools/GLQuad.cpp
    src/tools/ImageSaver.cpp
    src/ASsbpViewer.cpp
)

include_directories(PRIVATE
    ${PROJECT_SOURCE_DIR}
    "${PROJECT_SOURCE_DIR}/src"
    ${ImageMagick_INCLUDE_DIRS}
)

link_libraries(
    OpenGL::OpenGL
    glm::glm
    glfw
    "${ImageMagick_LIBRARIES}"
)

add_compile_options("${ImageMagick_COMPILE_OPTIONS}")

add_executable(SsbpPlayer
    ${SOURCES}
    src/SsbpViewer.cpp
)

add_custom_command(
    TARGET SsbpPlayer POST_BUILD
    #COMMAND $<IF:$<PLATFORM_ID:Windows>,dir,ls> "${ImageMagick_INCLUDE_DIRS}/.."
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:SsbpPlayer>" "${PROJECT_SOURCE_DIR}/bin"
)

add_executable(SsbpSaver
    ${SOURCES}
    src/SsbpSaver.cpp
)

set(SAVE_SPRITE "1" CACHE BOOL "Whether 'SsbpSaver' should save PNG image of animations' key frames.")
set(SAVE_ANIM "1" CACHE BOOL "Whether 'SsbpSaver' should save GIF image of whole animations.")

if (${SAVE_SPRITE})
    target_compile_definitions(SsbpSaver PUBLIC SAVE_SPRITE)
endif()
if (${SAVE_ANIM})
    target_compile_definitions(SsbpSaver PUBLIC SAVE_ANIM)
endif()

add_custom_command(
    TARGET SsbpSaver POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:SsbpSaver>" "${PROJECT_SOURCE_DIR}/bin"
)