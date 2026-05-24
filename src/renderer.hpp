#pragma once

#include <vector>
#include <glad/glad.h>
#include <glm/glm.hpp>

// ─────────────────────────────────────────────────────────────────────────
//  DrawMode
//    LINE   : GL_LINE_STRIP, uses line.vert / line.frag
//             Expects interleaved vertex data: x, y, speed  (3 floats/vertex)
//             Needs uSpeedMin / uSpeedMax uniforms.
//
//    POINTS : GL_POINTS with large point sprites, uses point.vert / point.frag
//             Expects vertex data: x, y  (2 floats/vertex)
// ─────────────────────────────────────────────────────────────────────────

enum class DrawMode { LINE, POINTS };

class Renderer
{
public:
    explicit Renderer(DrawMode mode,
                      const char* vertPath,
                      const char* fragPath);

    // Upload raw vertex data.
    // LINE mode   : pass { x0, y0, s0,  x1, y1, s1, ... }  (stride = 3)
    // POINTS mode : pass { x0, y0,  x1, y1, ... }           (stride = 2)
    void uploadData(const std::vector<float>& data);

    // Draw the uploaded geometry.
    // LINE mode   : also pass speedMin / speedMax for the colour ramp.
    void render(const glm::mat4& projection,
                float speedMin = 0.0f,
                float speedMax = 1.0f) const;

private:
    DrawMode mode_;
    GLuint   VAO_;
    GLuint   VBO_;
    GLuint   prog_;
    GLsizei  vertexCount_;

    void setupAttributes();
};