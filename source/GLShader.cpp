#include <glad/gl.h>
#include "GLShader.h"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace
{
    GLuint Compile(GLenum type, const std::string& path)
    {
        std::ifstream input(path);
        if (!input)
            throw std::runtime_error("Cannot open shader: " + path);
        std::ostringstream text;
        text << input.rdbuf();
        const std::string source = text.str();
        const char* ptr = source.c_str();
        const GLuint shader = glCreateShader(type);
        glShaderSource(shader, 1, &ptr, nullptr);
        glCompileShader(shader);
        GLint ok = 0;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
        if (!ok)
        {
            GLint length = 0;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
            std::vector<char> log(static_cast<size_t>(glm::max(length, 1)));
            glGetShaderInfoLog(shader, length, nullptr, log.data());
            glDeleteShader(shader);
            throw std::runtime_error("Shader compile failed: " + path + "\n" + log.data());
        }
        return shader;
    }
}

GLShader::GLShader(const std::string& vertexPath, const std::string& fragmentPath)
{
    const GLuint vertex = Compile(GL_VERTEX_SHADER, vertexPath);
    GLuint fragment = 0;
    try
    {
        fragment = Compile(GL_FRAGMENT_SHADER, fragmentPath);
        mProgram = glCreateProgram();
        glAttachShader(mProgram, vertex);
        glAttachShader(mProgram, fragment);
        glLinkProgram(mProgram);
        GLint ok = 0;
        glGetProgramiv(mProgram, GL_LINK_STATUS, &ok);
        if (!ok)
        {
            GLint length = 0;
            glGetProgramiv(mProgram, GL_INFO_LOG_LENGTH, &length);
            std::vector<char> log(static_cast<size_t>(glm::max(length, 1)));
            glGetProgramInfoLog(mProgram, length, nullptr, log.data());
            throw std::runtime_error(std::string("Shader link failed: ") + log.data());
        }
    }
    catch (...)
    {
        glDeleteShader(vertex);
        if (fragment) glDeleteShader(fragment);
        if (mProgram) glDeleteProgram(mProgram);
        throw;
    }
    glDeleteShader(vertex);
    glDeleteShader(fragment);
}

GLShader::~GLShader() { glDeleteProgram(mProgram); }
void GLShader::Use() const { glUseProgram(mProgram); }
void GLShader::Set(const char* name, int value) const { glUniform1i(glGetUniformLocation(mProgram, name), value); }
void GLShader::Set(const char* name, float value) const { glUniform1f(glGetUniformLocation(mProgram, name), value); }
void GLShader::Set(const char* name, const Vector3f& value) const
{ glUniform3fv(glGetUniformLocation(mProgram, name), 1, &value[0]); }
void GLShader::Set(const char* name, const Matrix4x4& value) const
{ glUniformMatrix4fv(glGetUniformLocation(mProgram, name), 1, GL_FALSE, &value[0][0]); }
