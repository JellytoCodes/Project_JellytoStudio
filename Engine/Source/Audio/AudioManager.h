#pragma once
#include "AudioClip.h"

namespace FMOD { class System; class Channel; }

class AudioManager
{
    DECLARE_SINGLE(AudioManager)

public:
    bool Init(const std::wstring& audioRootPath);
    void Update();
    void Shutdown();

    void PlayOneShot2D(const std::wstring& key);
    void PlayOneShot3D(const std::wstring& key, const Vec3& worldPos);

    void PlayBGM(const std::wstring& key);
    void StopBGM(bool immediate = false);
    void PauseBGM(bool pause);

    void SetListener(const Vec3& pos, const Vec3& forward, const Vec3& up);
    void SetMasterVolume(float volume);
    void SetBGMVolume(float volume);
    void SetSFXVolume(float volume);

private:
    struct AudioClipEntry
    {
        AudioClip clip;
    };

    AudioClipEntry* FindClip(const std::unordered_map<std::wstring, AudioManager::AudioClipEntry*>& clips, const std::wstring& key);

    bool LoadClip(const std::wstring& key, const std::wstring& fullPath,
        bool is3D, bool isLoop, float minDist, float maxDist);

    static constexpr int32 kMaxChannels = 64;

    FMOD::System* _pSystem = nullptr;
    FMOD::Channel* _pBGMChannel = nullptr;

    std::unordered_map<std::wstring, AudioClipEntry*> _clips;

    float _masterVolume = 1.0f;
    float _bgmVolume = 0.8f;
    float _sfxVolume = 1.0f;

    bool _initialized = false;
};