#pragma once
#include <glad/glad.h>
#include "Shader.h"

namespace eng {

    struct RGB { float r, g, b; };

    class Renderer {
    public:
        Renderer() = default;
        ~Renderer();

        void Init();
        void ComputeScale(int fbw, int fbh, int boardW, int boardH);

        void Quad(float cx, float cy, float sx, float sy, RGB col) const;

        // Board metrics for HUD anchoring
        float cellW = 0.0f, cellH = 0.0f;
        float left = 0.0f, bottom = -1.0f, boardRight = 0.0f, boardTop = 0.0f;

        Shader& Program() { return m_Shader; }

    private:
        Shader m_Shader;
        GLuint m_VAO = 0, m_VBO = 0;
    };

} // namespace eng
