#include "Texture.h"

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#include "../../lib/stb/stb_image.h"            // <<— make path explicit
#include <cstdio>

namespace eng {

    bool Texture2D::LoadRGBA(const char* path) {
        int comp = 0;
        unsigned char* data = stbi_load(path, &w, &h, &comp, 4);
        if (!data) { std::fprintf(stderr, "[Texture] load fail: %s\n", path); return false; }
        glGenTextures(1, &id);
        glBindTexture(GL_TEXTURE_2D, id);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glBindTexture(GL_TEXTURE_2D, 0);
        stbi_image_free(data);
        return true;
    }
    void Texture2D::Destroy() { if (id) { glDeleteTextures(1, &id); id = 0; } }

} // namespace eng
