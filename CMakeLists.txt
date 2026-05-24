cmake_minimum_required(VERSION 3.16)

project(projectile_opengl)

# =========================================================
# C++ STANDARD
# =========================================================

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# =========================================================
# FIND PACKAGES
# =========================================================

find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)

# =========================================================
# INCLUDE DIRECTORIES
# =========================================================

include_directories(
    /usr/include/eigen3
    external/include
)

# =========================================================
# EXECUTABLE
# =========================================================

add_executable(projectile

    # main
    src/main.cpp

    # physics
    src/physics.cpp

    # rendering
    src/renderer.cpp
    src/shader.cpp

    # glad
    external/include/glad/src/glad.c
)

# =========================================================
# LINK LIBRARIES
# =========================================================

target_link_libraries(projectile
    OpenGL::GL
    glfw
)

# =========================================================
# COPY SHADERS TO BUILD DIRECTORY
# =========================================================

file(COPY shaders DESTINATION ${CMAKE_BINARY_DIR})