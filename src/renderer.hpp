#pragma once

#include <vector>

#include <glad/glad.h>

#include <glm/glm.hpp>

class Renderer
{
public:

    Renderer();

    void uploadTrajectory(
        const std::vector<float>& vertices
    );

    void render(
        const glm::mat4& projection
    );

private:

    GLuint VAO;
    GLuint VBO;

    GLuint shaderProgram;

    GLsizei vertexCount;
};