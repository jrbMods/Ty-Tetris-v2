#pragma once
#include <string>
#include <unordered_map>
#include <glad/glad.h>

namespace eng {

    class Shader {
    public:
        Shader() = default;
        ~Shader();

        static Shader FromSource(const char* vs, const char* fs);

        void Bind() const;
        void Unbind() const;

        // Uniform helpers (const so we can call from const draw)
        void SetFloat(const char* name, float v) const;
        void SetFloat2(const char* name, float x, float y) const;
        void SetFloat3(const char* name, float x, float y, float z) const;
        void SetMat4(const char* name, const float* m16) const;

    private:
        GLuint m_ID = 0;
        mutable std::unordered_map<std::string, GLint> m_Locs;

        GLint GetLoc(const char* name) const;
    };

} // namespace eng
