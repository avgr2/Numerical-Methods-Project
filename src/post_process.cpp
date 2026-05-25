#include "post_process.hpp"
#include "shader.hpp"

#include <stdexcept>
#include <iostream>

// ─────────────────────────────────────────────────────────────────────────
PostProcess::PostProcess(int width, int height)
    : w_(width), h_(height)
{
    buildShaders();

    // Empty VAO for the gl_VertexID fullscreen triangle trick
    glGenVertexArrays(1, &emptyVAO_);

    // Create the four FBOs
    createFBO(sceneFBO_,   sceneTexture_);
    createFBO(extractFBO_, extractTexture_);
    createFBO(pingFBO_,    pingTexture_);
    createFBO(pongFBO_,    pongTexture_);
}

PostProcess::~PostProcess()
{
    deleteFBOs();
    glDeleteVertexArrays(1, &emptyVAO_);
}

// ─────────────────────────────────────────────────────────────────────────
void PostProcess::createFBO(GLuint& fbo, GLuint& tex)
{
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F,
                 w_, h_, 0, GL_RGB, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                           GL_TEXTURE_2D, tex, 0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cerr << "PostProcess: FBO not complete!\n";

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

// ─────────────────────────────────────────────────────────────────────────
void PostProcess::deleteFBOs()
{
    glDeleteFramebuffers(1, &sceneFBO_);
    glDeleteFramebuffers(1, &extractFBO_);
    glDeleteFramebuffers(1, &pingFBO_);
    glDeleteFramebuffers(1, &pongFBO_);

    glDeleteTextures(1, &sceneTexture_);
    glDeleteTextures(1, &extractTexture_);
    glDeleteTextures(1, &pingTexture_);
    glDeleteTextures(1, &pongTexture_);
}

// ─────────────────────────────────────────────────────────────────────────
void PostProcess::buildShaders()
{
    progBackground_ = createShaderProgram("shaders/quad_uv.vert",
                                          "shaders/background.frag");
    progExtract_    = createShaderProgram("shaders/quad_uv.vert",
                                          "shaders/bloom_extract.frag");
    progBlur_       = createShaderProgram("shaders/quad_uv.vert",
                                          "shaders/bloom_blur.frag");
    progComposite_  = createShaderProgram("shaders/quad_uv.vert",
                                          "shaders/bloom_composite.frag");
}

// ─────────────────────────────────────────────────────────────────────────
void PostProcess::drawFullscreenTriangle()
{
    glBindVertexArray(emptyVAO_);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);
}

// ─────────────────────────────────────────────────────────────────────────
void PostProcess::bindScene()
{
    glBindFramebuffer(GL_FRAMEBUFFER, sceneFBO_);
    glViewport(0, 0, w_, h_);

    // Draw background shader instead of plain glClear
    // (we still need to clear depth if you add it later)
    glDisable(GL_DEPTH_TEST);

    glUseProgram(progBackground_);
    drawFullscreenTriangle();
}

// ─────────────────────────────────────────────────────────────────────────
void PostProcess::process(float threshold, float intensity)
{
    glDisable(GL_DEPTH_TEST);

    // ── Pass 1 : extract bright pixels ──────────────────────────────
    glBindFramebuffer(GL_FRAMEBUFFER, extractFBO_);
    glViewport(0, 0, w_, h_);

    glUseProgram(progExtract_);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, sceneTexture_);
    glUniform1i(glGetUniformLocation(progExtract_, "uScene"), 0);
    glUniform1f(glGetUniformLocation(progExtract_, "uThreshold"), threshold);
    drawFullscreenTriangle();

    // ── Pass 2 : horizontal Gaussian blur ───────────────────────────
    glBindFramebuffer(GL_FRAMEBUFFER, pingFBO_);
    glViewport(0, 0, w_, h_);

    glUseProgram(progBlur_);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, extractTexture_);
    glUniform1i(glGetUniformLocation(progBlur_, "uBlurInput"), 0);
    glUniform2f(glGetUniformLocation(progBlur_, "uDirection"),
                1.0f / (float)w_, 0.0f);
    drawFullscreenTriangle();

    // ── Pass 3 : vertical Gaussian blur ─────────────────────────────
    glBindFramebuffer(GL_FRAMEBUFFER, pongFBO_);
    glViewport(0, 0, w_, h_);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, pingTexture_);
    glUniform2f(glGetUniformLocation(progBlur_, "uDirection"),
                0.0f, 1.0f / (float)h_);
    drawFullscreenTriangle();

    // ── Pass 4 : composite scene + bloom → screen ───────────────────
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, w_, h_);

    glUseProgram(progComposite_);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, sceneTexture_);
    glUniform1i(glGetUniformLocation(progComposite_, "uScene"), 0);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, pongTexture_);
    glUniform1i(glGetUniformLocation(progComposite_, "uBloom"), 1);
    glUniform1f(glGetUniformLocation(progComposite_, "uIntensity"), intensity);
    drawFullscreenTriangle();
}

// ─────────────────────────────────────────────────────────────────────────
void PostProcess::resize(int width, int height)
{
    w_ = width;
    h_ = height;
    deleteFBOs();
    createFBO(sceneFBO_,   sceneTexture_);
    createFBO(extractFBO_, extractTexture_);
    createFBO(pingFBO_,    pingTexture_);
    createFBO(pongFBO_,    pongTexture_);
}