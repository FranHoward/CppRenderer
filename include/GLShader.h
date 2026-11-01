#pragma once
#include "Common.h"
#include <string>

// Must be created/destroyed while the OpenGL context is current.
class GLShader
{
public:
    GLShader(const std::string& vertexPath, const std::string& fragmentPath);
    ~GLShader();
    GLShader(const GLShader&) = delete;
    GLShader& operator=(const GLShader&) = delete;
    void Use() const;
    void Set(const char* name, int value) const;
    void Set(const char* name, float value) const;
    void Set(const char* name, const Vector3f& value) const;
    void Set(const char* name, const Matrix4x4& value) const;
private:
    unsigned int mProgram = 0;
};
