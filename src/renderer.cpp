#include "renderer.hpp"
#include "shader.hpp"

#include <glm/gtc/type_ptr.hpp>
#include <stdexcept>

Renderer::Renderer(DrawMode mode,
                   const char* vertPath,
                   const char* fragPath)
    : mode_(mode), vertexCount_(0)
{
    prog_ = createShaderProgram(vertPath, fragPath);
    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);
    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    setupAttributes();
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

Renderer::~Renderer()
{
    glDeleteBuffers(1, &vbo_);
    glDeleteVertexArrays(1, &vao_);
}

int Renderer::stride() const
{
    switch (mode_) {
        case DrawMode::POINTS:    return 2;
        case DrawMode::LINES:     return 3;   // x,y,type
        case DrawMode::LINE_STRIP:return 3;   // x,y,speed
        case DrawMode::COMET:     return 3;   // x,y,alpha
    }
    return 2;
}

void Renderer::setupAttributes()
{
    if (mode_ == DrawMode::POINTS)
    {
        // location 0 : vec2 position
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE,
                              2*sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
    }
    else
    {
        // location 0 : vec2 position
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE,
                              3*sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        // location 1 : float (type / speed / alpha)
        glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE,
                              3*sizeof(float), (void*)(2*sizeof(float)));
        glEnableVertexAttribArray(1);
    }
}

void Renderer::uploadData(const std::vector<float>& data)
{
    vertexCount_ = (GLsizei)(data.size() / stride());
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER,
                 data.size()*sizeof(float),
                 data.data(), GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void Renderer::render(const glm::mat4& projection,
                      float param0, float param1) const
{
    if (vertexCount_ == 0) return;

    glUseProgram(prog_);
    glUniformMatrix4fv(glGetUniformLocation(prog_, "uProjection"),
                       1, GL_FALSE, glm::value_ptr(projection));

    if (mode_ == DrawMode::LINE_STRIP)
    {
        glUniform1f(glGetUniformLocation(prog_, "uSpeedMin"), param0);
        glUniform1f(glGetUniformLocation(prog_, "uSpeedMax"), param1);
    }

    glBindVertexArray(vao_);

    switch (mode_)
    {
    case DrawMode::LINES:
        glLineWidth(1.0f);
        glDrawArrays(GL_LINES, 0, vertexCount_);
        break;

    case DrawMode::LINE_STRIP:
        glLineWidth(2.5f);
        glDrawArrays(GL_LINE_STRIP, 0, vertexCount_);
        glLineWidth(1.0f);
        break;

    case DrawMode::POINTS:
        glEnable(GL_PROGRAM_POINT_SIZE);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDrawArrays(GL_POINTS, 0, vertexCount_);
        glDisable(GL_BLEND);
        glDisable(GL_PROGRAM_POINT_SIZE);
        break;

    case DrawMode::COMET:
        glEnable(GL_PROGRAM_POINT_SIZE);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);   // additive for glow
        glDrawArrays(GL_POINTS, 0, vertexCount_);
        glDisable(GL_BLEND);
        glDisable(GL_PROGRAM_POINT_SIZE);
        break;
    }

    glBindVertexArray(0);
}