#pragma once

#include <vector>
#include <glad/glad.h>
#include <glm/glm.hpp>

// ─────────────────────────────────────────────────────────────────────────
//  DrawMode
//
//  LINES   : GL_LINES  (explicit pairs) – used for the grid
//            Stride = 3 floats : x, y, type
//
//  LINE_STRIP : GL_LINE_STRIP – trajectory colour ramp
//            Stride = 3 floats : x, y, speed
//
//  POINTS  : GL_POINTS with large sprites – observations & comet
//            Stride = 2 floats : x, y
//            (comet adds alpha as 3rd float → stride = 3)
//
//  COMET   : GL_POINTS with alpha attribute
//            Stride = 3 floats : x, y, alpha
// ─────────────────────────────────────────────────────────────────────────

enum class DrawMode { LINES, LINE_STRIP, POINTS, COMET };

class Renderer
{
public:
    explicit Renderer(DrawMode mode,
                      const char* vertPath,
                      const char* fragPath);
    ~Renderer();

    void uploadData(const std::vector<float>& data);

    void render(const glm::mat4& projection,
                float param0 = 0.0f,   // speedMin  (LINE_STRIP)
                float param1 = 1.0f    // speedMax  (LINE_STRIP)
               ) const;

private:
    DrawMode mode_;
    GLuint   vao_, vbo_, prog_;
    GLsizei  vertexCount_;

    int stride() const;
    void setupAttributes();
};