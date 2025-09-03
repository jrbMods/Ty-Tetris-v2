#include "Shader.h"
#include <cstdio>

namespace eng {

    static GLuint Compile(GLenum type, const char* src) {
        GLuint s = glCreateShader(type);
        glShaderSource(s, 1, &src, nullptr);
        glCompileShader(s);
        GLint ok = 0; glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
        if (!ok) {
            char log[2048]; GLsizei n = 0; glGetShaderInfoLog(s, 2048, &n, log);
            std::fprintf(stderr, "[Shader] compile error:\n%.*s\n", n, log);
        }
        return s;
    }

    Shader Shader::FromSource(const char* vs, const char* fs) {
        Shader sh;
        GLuint v = Compile(GL_VERTEX_SHADER, vs);
        GLuint f = Compile(GL_FRAGMENT_SHADER, fs);
        sh.m_ID = glCreateProgram();
        glAttachShader(sh.m_ID, v);
        glAttachShader(sh.m_ID, f);
        glLinkProgram(sh.m_ID);
        GLint ok = 0; glGetProgramiv(sh.m_ID, GL_LINK_STATUS, &ok);
        if (!ok) {
            char log[2048]; GLsizei n = 0; glGetProgramInfoLog(sh.m_ID, 2048, &n, log);
            std::fprintf(stderr, "[Shader] link error:\n%.*s\n", n, log);
        }
        glDeleteShader(v); glDeleteShader(f);
        return sh;
    }

    Shader::~Shader() { if (m_ID) glDeleteProgram(m_ID); }

    void Shader::Bind() const { glUseProgram(m_ID); }
    void Shader::Unbind() const { glUseProgram(0); }

    GLint Shader::GetLoc(const char* name) const {
        auto it = m_Locs.find(name);
        if (it != m_Locs.end()) return it->second;
        GLint loc = glGetUniformLocation(m_ID, name);
        m_Locs[name] = loc;
        return loc;
    }

    void Shader::SetFloat(const char* n, float v)                    const { glUniform1f(GetLoc(n), v); }
    void Shader::SetFloat2(const char* n, float x, float y)           const { glUniform2f(GetLoc(n), x, y); }
    void Shader::SetFloat3(const char* n, float x, float y, float z)  const { glUniform3f(GetLoc(n), x, y, z); }
    void Shader::SetMat4(const char* n, const float* m16)           const { glUniformMatrix4fv(GetLoc(n), 1, GL_FALSE, m16); }

} // namespace eng
