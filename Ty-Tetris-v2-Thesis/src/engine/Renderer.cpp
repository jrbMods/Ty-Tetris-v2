#include "Renderer.h"

namespace eng {

    static const char* VS_SRC =
        "#version 330 core\n"
        "layout(location=0) in vec2 aPos;\n"
        "uniform vec2 uScale; uniform vec2 uOffset;\n"
        "void main(){ gl_Position = vec4(aPos*uScale + uOffset, 0.0, 1.0); }\n";

    static const char* FS_SRC =
        "#version 330 core\n"
        "out vec4 FragColor; uniform vec3 uColor;\n"
        "void main(){ FragColor = vec4(uColor,1.0); }\n";

    Renderer::~Renderer() {
        if (m_VBO) glDeleteBuffers(1, &m_VBO);
        if (m_VAO) glDeleteVertexArrays(1, &m_VAO);
    }

    void Renderer::Init() {
        m_Shader = Shader::FromSource(VS_SRC, FS_SRC);

        glGenVertexArrays(1, &m_VAO);
        glBindVertexArray(m_VAO);
        glGenBuffers(1, &m_VBO);
        glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
        const float verts[12] = {
            -0.5f,-0.5f,  0.5f,-0.5f,  0.5f, 0.5f,
            -0.5f,-0.5f,  0.5f, 0.5f, -0.5f, 0.5f
        };
        glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
        glBindVertexArray(0);

        // Ensure nothing hides our 2D quads
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);
        glDisable(GL_SCISSOR_TEST);
    }

    void Renderer::ComputeScale(int fbw, int fbh, int boardW, int boardH) {
        cellH = 2.0f / float(boardH);
        float aspect = float(fbh) / float(fbw);
        cellW = cellH * aspect;
        left = -(boardW * cellW) * 0.5f;
        bottom = -1.0f;
        boardRight = left + boardW * cellW;
        boardTop = bottom + boardH * cellH;
    }

    void Renderer::Quad(float cx, float cy, float sx, float sy, RGB col) const {
        m_Shader.Bind();
        m_Shader.SetFloat2("uScale", sx, sy);
        m_Shader.SetFloat2("uOffset", cx, cy);
        m_Shader.SetFloat3("uColor", col.r, col.g, col.b);
        glBindVertexArray(m_VAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);
    }

} // namespace eng
