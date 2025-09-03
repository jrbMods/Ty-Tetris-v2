#pragma once
#include <glad/glad.h>

namespace eng {

    class Texture2D {
    public:
        GLuint id = 0; int w = 0, h = 0;

        bool LoadRGBA(const char* path); // via stb_image
        void Destroy();
    };

} // namespace eng
