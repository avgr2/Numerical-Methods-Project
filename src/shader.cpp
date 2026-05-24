#include "shader.hpp"

#include <fstream>
#include <sstream>
#include <iostream>

#include <glad/glad.h>

static std::string readFile(const std::string& path)
{
    std::ifstream file(path);

    std::stringstream ss;
    ss << file.rdbuf();

    return ss.str();
}

unsigned int createShaderProgram(
    const std::string& vertexPath,
    const std::string& fragmentPath
)
{
    std::string vertSrc = readFile(vertexPath);
    std::string fragSrc = readFile(fragmentPath);

    const char* vsrc = vertSrc.c_str();
    const char* fsrc = fragSrc.c_str();

    GLuint vs = glCreateShader(GL_VERTEX_SHADER);

    glShaderSource(vs, 1, &vsrc, nullptr);
    glCompileShader(vs);

    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);

    glShaderSource(fs, 1, &fsrc, nullptr);
    glCompileShader(fs);

    GLuint program = glCreateProgram();

    glAttachShader(program, vs);
    glAttachShader(program, fs);

    glLinkProgram(program);

    glDeleteShader(vs);
    glDeleteShader(fs);

    return program;
}