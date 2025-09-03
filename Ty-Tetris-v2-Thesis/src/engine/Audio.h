#pragma once

struct ma_engine;
struct ma_sound;

namespace eng {

    class Audio {
    public:
        Audio();
        ~Audio();

        bool Init();
        void Shutdown();

        bool PlayMusic(const char* path, bool loop);
        void StopMusic();
        void SetMusicOn(bool on);

        bool IsMusicOn() const { return m_MusicOn; }

    private:
        ma_engine* m_Engine = nullptr;
        ma_sound* m_Music = nullptr;
        bool       m_MusicOn = true;
    };

} // namespace eng
