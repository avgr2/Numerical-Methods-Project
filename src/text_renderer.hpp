#pragma once

#include <string>
#include <vector>
#include <glad/glad.h>
#include <glm/glm.hpp>

// ─────────────────────────────────────────────────────────────────────────
//  TextRenderer
//
//  Screen-space bitmap text using a self-contained 8×8 ASCII glyph atlas.
//  No external font library required.
//
//  Usage:
//      TextRenderer tr(windowWidth, windowHeight);
//      tr.draw("mu = 0.00312", 20, 20, 2.0f, {1,1,0});
//
//  Coordinates are in pixels from the top-left corner of the window.
//  scale = 1.0 → 8×8 px glyphs,  scale = 2.0 → 16×16 px, etc.
// ─────────────────────────────────────────────────────────────────────────

class TextRenderer
{
public:
    TextRenderer(int windowW, int windowH,
                 const char* vertPath = "shaders/text.vert",
                 const char* fragPath = "shaders/text.frag");

    ~TextRenderer();

    // Draw a string at pixel position (px, py) from top-left.
    // color is an RGB vec3  (e.g. {1,1,1} = white).
    void draw(const std::string& text,
              float px, float py,
              float scale,
              glm::vec3 color = {1.0f, 1.0f, 1.0f}) const;

    // Call this when the window is resized.
    void resize(int w, int h);

private:
    int    winW_, winH_;
    GLuint prog_;
    GLuint vao_, vbo_;
    GLuint atlasTexture_;

    void buildAtlas();
    void uploadQuads(const std::vector<float>& verts) const;
};