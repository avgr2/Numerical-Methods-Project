#include "renderer.hpp"
#include "shader.hpp"

#include <glm/gtc/type_ptr.hpp>

Renderer::Renderer()
{
    shaderProgram = createShaderProgram(
        "shaders/line.vert",
        "shaders/line.frag"
    );

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    glVertexAttribPointer(
        0,
        2,
        GL_FLOAT,
        GL_FALSE,
        2 * sizeof(float),
        (void*)0
    );

    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    vertexCount = 0;
}

void Renderer::uploadTrajectory(
    const std::vector<float>& vertices
)
{
    vertexCount = vertices.size() / 2;

    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    glBufferData(
        GL_ARRAY_BUFFER,
        vertices.size() * sizeof(float),
        vertices.data(),
        GL_STATIC_DRAW
    );
}

void Renderer::render(
    const glm::mat4& projection
)
{
    glUseProgram(shaderProgram);

    GLint projLoc =
        glGetUniformLocation(shaderProgram, "uProjection");

    glUniformMatrix4fv(
        projLoc,
        1,
        GL_FALSE,
        glm::value_ptr(projection)
    );

    glBindVertexArray(VAO);

    glDrawArrays(
        GL_LINE_STRIP,
        0,
        vertexCount
    );
}