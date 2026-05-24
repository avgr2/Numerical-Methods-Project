#include <iostream>
#include <vector>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "physics.hpp"
#include "renderer.hpp"

int main()
{
    // =====================================================
    // GLFW INIT
    // =====================================================

    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

    glfwWindowHint(
        GLFW_OPENGL_PROFILE,
        GLFW_OPENGL_CORE_PROFILE
    );

    // =====================================================
    // WINDOW
    // =====================================================

    GLFWwindow* window =
        glfwCreateWindow(
            1280,
            720,
            "Projectile Simulation",
            nullptr,
            nullptr
        );

    if (!window) {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    // =====================================================
    // GLAD
    // =====================================================

    if (!gladLoadGLLoader(
        (GLADloadproc)glfwGetProcAddress))
    {
        std::cerr << "Failed to initialize GLAD\n";
        return -1;
    }

    // =====================================================
    // OPENGL CONFIG
    // =====================================================

    glViewport(0, 0, 1280, 720);

    glClearColor(
        0.05f,
        0.05f,
        0.08f,
        1.0f
    );

    // =====================================================
    // PHYSICS
    // =====================================================

    Physics physics;

    Params params;

    params.x0 = 0.0;
    params.y0 = 0.0;
    params.v0 = 200.0;
    params.theta0 = 45.0;
    params.mu = 0.9;
    params.g = 9.81;

    auto trajectory =
        physics.simulate(
            params,
            0.01,
            20.0
        );

    // =====================================================
    // CPU -> GPU VERTICES
    // =====================================================

    std::vector<float> vertices;

    for (const auto& p : trajectory)
    {
        vertices.push_back(p.x);
        vertices.push_back(p.y);
    }

    // =====================================================
    // RENDERER
    // =====================================================

    Renderer renderer;

    renderer.uploadTrajectory(vertices);

    // =====================================================
    // PROJECTION MATRIX
    // =====================================================

    glm::mat4 projection =
        glm::ortho(
            0.0f,
            200.0f,
            0.0f,
            150.0f
        );

    // =====================================================
    // MAIN LOOP
    // =====================================================

    while (!glfwWindowShouldClose(window))
    {
        glClear(GL_COLOR_BUFFER_BIT);

        renderer.render(projection);

        glfwSwapBuffers(window);

        glfwPollEvents();
    }

    // =====================================================
    // CLEANUP
    // =====================================================

    glfwTerminate();

    return 0;
}