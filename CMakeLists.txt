cmake_minimum_required(VERSION 3.9)
project(hmaprenderer)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(CMAKE_CXX_STANDARD 11)
find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(GLM REQUIRED)
find_package(GLU REQUIRED)
find_package(GLFW REQUIRED)

add_executable(hmaprenderer main.cpp shader.cpp shader.h)
target_link_libraries(
        hmaprenderer
        ${OpenGL_LIBRARY}
        ${GLFW_LIBRARY}
        ${GLEW_LIBRARY}
        ${GLM_LIBRARY}
        ${GLU_LIBRARY}
        -lGL
)