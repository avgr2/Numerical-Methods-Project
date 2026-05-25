#pragma once

#include <glad/glad.h>

// ─────────────────────────────────────────────────────────────────────────
//  PostProcess
//
//  Manages the full bloom post-processing pipeline:
//
//    1. sceneFBO    – render the entire scene into this texture
//    2. extractFBO  – extract bright pixels (luminance threshold)
//    3. pingFBO     – horizontal Gaussian blur
//    4. pongFBO     – vertical Gaussian blur  (result = bloom layer)
//    5. Composite   – scene + bloom → default framebuffer
//
//  Usage:
//
//      PostProcess pp(1280, 720);
//
//      // Scene render pass
//      pp.bindScene();
//      ... render everything ...
//
//      // Post-process + present to screen
//      pp.process(bloomThreshold, bloomIntensity);
// ─────────────────────────────────────────────────────────────────────────

class PostProcess
{
public:
    PostProcess(int width, int height);
    ~PostProcess();

    // Bind the scene FBO – render your scene into this
    void bindScene();

    // Run extract → blur H → blur V → composite, output to screen
    void process(float threshold = 0.35f, float intensity = 1.2f);

    // Call when window is resized
    void resize(int width, int height);

private:
    int w_, h_;

    // Framebuffer objects
    GLuint sceneFBO_,   sceneTexture_;
    GLuint extractFBO_, extractTexture_;
    GLuint pingFBO_,    pingTexture_;
    GLuint pongFBO_,    pongTexture_;

    // Shader programs
    GLuint progExtract_;
    GLuint progBlur_;
    GLuint progComposite_;
    GLuint progBackground_;

    // Empty VAO for fullscreen triangle draw (gl_VertexID trick)
    GLuint emptyVAO_;

    void createFBO(GLuint& fbo, GLuint& tex);
    void deleteFBOs();
    void buildShaders();
    void drawFullscreenTriangle();
};