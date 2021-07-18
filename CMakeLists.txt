cmake_minimum_required(VERSION 3.0.0)

set(PROJECT ssbpViewer)
project(${PROJECT})

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(MSVC_DISABLED_WARNINGS_LIST
    "C4251"
)

find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)
find_package(ImageMagick COMPONENTS Magick++ REQUIRED)
find_package(ImageMagick COMPONENTS MagickCore REQUIRED)
find_package(ImageMagick COMPONENTS MagickWand REQUIRED)

#if(WIN32)
#add_compile_options(/W4 /D_WIN32_WINNT=0x0601)
#else()
#add_compile_options(-Wall -Wextra -O2)
#endif()

set(CMAKE_INCLUDE_CURRENT_DIR ON)

file(GLOB SOURCES
    src/glad.c
    ssbp/Common/*/*.cpp
    ssbp/SS5PlayerPlatform.cpp
    src/shader.cpp
    src/sprite.cpp
    src/texture.cpp
)

include_directories(
    include
    ${ImageMagick_INCLUDE_DIRS}
)

link_libraries(
    opengl32
    glfw
    glm::glm
    ${ImageMagick_LIBRARIES}
)

add_executable(${PROJECT}
    ${SOURCES}
    src/main.cpp
)
set_target_properties(${PROJECT} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${PROJECT_SOURCE_DIR}"#"${CMAKE_BINARY_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${PROJECT_SOURCE_DIR}"
    DEBUG_POSTFIX "_d"
)

#if(UNIX)
#    target_link_libraries(${PROJECT} PUBLIC dl)
#endif()

#target_link_libraries(${PROJECT} PUBLIC
#    "C:/Program Files/GLEW/lib/glew32.lib"
#)