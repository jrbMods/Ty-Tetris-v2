#include "Audio.h"
#define MINIAUDIO_IMPLEMENTATION
#include "../../vendor/miniaudio/miniaudio.h"   // <<— make path explicit
#include <cstdio>

namespace eng {

    Audio::Audio() = default;
    Audio::~Audio() { Shutdown(); }

    bool Audio::Init() {
        if (m_Engine) return true;
        m_Engine = new ma_engine();
        if (ma_engine_init(nullptr, m_Engine) != MA_SUCCESS) {
            std::fprintf(stderr, "[Audio] engine init failed\n");
            delete m_Engine; m_Music = nullptr; m_Engine = nullptr;
            return false;
        }
        return true;
    }

    void Audio::Shutdown() {
        if (m_Music) { ma_sound_uninit(m_Music); delete m_Music; m_Music = nullptr; }
        if (m_Engine) { ma_engine_uninit(m_Engine); delete m_Engine; m_Engine = nullptr; }
    }

    bool Audio::PlayMusic(const char* path, bool loop) {
        if (!m_Engine) if (!Init()) return false;
        if (m_Music) { ma_sound_uninit(m_Music); delete m_Music; m_Music = nullptr; }
        m_Music = new ma_sound();
        if (ma_sound_init_from_file(m_Engine, path, 0, nullptr, nullptr, m_Music) != MA_SUCCESS) {
            std::fprintf(stderr, "[Audio] music load fail: %s\n", path);
            delete m_Music; m_Music = nullptr; return false;
        }
        ma_sound_set_looping(m_Music, loop ? MA_TRUE : MA_FALSE);
        if (m_MusicOn) ma_sound_start(m_Music);
        return true;
    }

    void Audio::StopMusic() {
        if (m_Music) ma_sound_stop(m_Music);
    }

    void Audio::SetMusicOn(bool on) {
        m_MusicOn = on;
        if (!m_Music) return;
        if (on) ma_sound_start(m_Music);
        else    ma_sound_stop(m_Music);
    }

} // namespace eng
