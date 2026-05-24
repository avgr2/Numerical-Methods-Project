#include "renderer.hpp"
#include "shader.hpp"

#include <glm/gtc/type_ptr.hpp>
#include <stdexcept>

// ─────────────────────────────────────────────────────────────────────────
//  Constructor
// ─────────────────────────────────────────────────────────────────────────
Renderer::Renderer(DrawMode mode,
                   const char* vertPath,
                   const char* fragPath)
    : mode_(mode), vertexCount_(0)
{
    prog_ = createShaderProgram(vertPath, fragPath);

    glGenVertexArrays(1, &VAO_);
    glGenBuffers(1, &VBO_);

    glBindVertexArray(VAO_);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_);

    setupAttributes();

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

// ─────────────────────────────────────────────────────────────────────────
//  setupAttributes  – configure VAO layout depending on mode
// ─────────────────────────────────────────────────────────────────────────
void Renderer::setupAttributes()
{
    if (mode_ == DrawMode::LINE)
    {
        // Stride = 3 floats : [ x, y, speed ]
        const GLsizei stride = 3 * sizeof(float);

        // location 0 : position (x, y)
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE,
                              stride, (void*)0);
        glEnableVertexAttribArray(0);

        // location 1 : speed
        glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE,
                              stride, (void*)(2 * sizeof(float)));
        glEnableVertexAttribArray(1);
    }
    else   // POINTS
    {
        // Stride = 2 floats : [ x, y ]
        const GLsizei stride = 2 * sizeof(float);

        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE,
                              stride, (void*)0);
        glEnableVertexAttribArray(0);
    }
}

// ─────────────────────────────────────────────────────────────────────────
//  uploadData
// ─────────────────────────────────────────────────────────────────────────
void Renderer::uploadData(const std::vector<float>& data)
{
    const int stride = (mode_ == DrawMode::LINE) ? 3 : 2;
    vertexCount_     = static_cast<GLsizei>(data.size() / stride);

    glBindBuffer(GL_ARRAY_BUFFER, VBO_);
    glBufferData(GL_ARRAY_BUFFER,
                 data.size() * sizeof(float),
                 data.data(),
                 GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

// ─────────────────────────────────────────────────────────────────────────
//  render
// ─────────────────────────────────────────────────────────────────────────
void Renderer::render(const glm::mat4& projection,
                      float speedMin,
                      float speedMax) const
{
    if (vertexCount_ == 0) return;

    glUseProgram(prog_);

    // Projection matrix (common to both shaders)
    GLint loc = glGetUniformLocation(prog_, "uProjection");
    glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(projection));

    if (mode_ == DrawMode::LINE)
    {
        glUniform1f(glGetUniformLocation(prog_, "uSpeedMin"), speedMin);
        glUniform1f(glGetUniformLocation(prog_, "uSpeedMax"), speedMax);
    }

    glBindVertexArray(VAO_);

    if (mode_ == DrawMode::LINE)
    {
        // Slightly thicker line for visibility
        glLineWidth(2.0f);
        glDrawArrays(GL_LINE_STRIP, 0, vertexCount_);
        glLineWidth(1.0f);
    }
    else  // POINTS
    {
        // Enable point-size set in vertex shader and alpha blending
        glEnable(GL_PROGRAM_POINT_SIZE);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glDrawArrays(GL_POINTS, 0, vertexCount_);

        glDisable(GL_BLEND);
        glDisable(GL_PROGRAM_POINT_SIZE);
    }

    glBindVertexArray(0);
}