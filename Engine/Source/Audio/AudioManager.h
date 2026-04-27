#pragma once
#include "Audio/SoundEvent.h"

namespace FMOD { class System; class Channel; }

class AudioManager
{
    DECLARE_SINGLE(AudioManager)

public:
    bool Init(const std::wstring& assetRootPath);
    void Update();
    void Shutdown();

    void PlayOneShot2D(SoundEvent ev);
    void PlayOneShot3D(SoundEvent ev, const Vec3& worldPos);

    void SetListener(const Vec3& pos, const Vec3& forward, const Vec3& up);

    void SetMasterVolume(float volume);
    void SetBGMVolume(float volume);
    void SetSFXVolume(float volume);

    void PlayBGM();
    void StopBGM(bool immediate = false);
    void PauseBGM(bool pause);

private:
    bool LoadClip(SoundEvent ev, const std::wstring& filePath, bool is3D, bool isLoop);

    static constexpr int32 kMaxChannels = 64;
    static constexpr float kDefault3DMinDist = 1.0f;
    static constexpr float kDefault3DMaxDist = 30.0f;

    FMOD::System* _pSystem = nullptr;
    FMOD::Channel* _pBGMChannel = nullptr;

    struct AudioClipEntry;
    std::array<AudioClipEntry*, static_cast<uint32>(SoundEvent::Count)> _clips = {};

    float _masterVolume = 1.0f;
    float _bgmVolume = 0.8f;
    float _sfxVolume = 1.0f;

    bool _initialized = false;
};
