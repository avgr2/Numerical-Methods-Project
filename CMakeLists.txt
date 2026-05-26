cmake_minimum_required(VERSION 3.16)
project(projectile)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── Find packages ─────────────────────────────────────────────────────────
find_package(OpenGL REQUIRED)
find_package(glfw3  REQUIRED)
find_package(Eigen3 REQUIRED)

# ── Source files ──────────────────────────────────────────────────────────
set(SOURCES
    src/main.cpp
    src/physics.cpp
    src/renderer.cpp
    src/shader.cpp
    src/text_renderer.cpp
    src/post_process.cpp
    src/heatmap_view.cpp
    external/include/glad/src/glad.c
)

add_executable(projectile ${SOURCES})

# ── Include directories ───────────────────────────────────────────────────
target_include_directories(projectile PRIVATE
    external/include
    external/include/glad
    ${EIGEN3_INCLUDE_DIR}
)

# ── Link libraries ────────────────────────────────────────────────────────
target_link_libraries(projectile PRIVATE
    OpenGL::GL
    glfw
    Eigen3::Eigen
)

# ── Copy shaders to build directory ──────────────────────────────────────
file(GLOB SHADER_FILES "${CMAKE_SOURCE_DIR}/shaders/*")
foreach(SHADER ${SHADER_FILES})
    get_filename_component(SHADER_NAME ${SHADER} NAME)
    configure_file(${SHADER} ${CMAKE_BINARY_DIR}/shaders/${SHADER_NAME} COPYONLY)
endforeach()