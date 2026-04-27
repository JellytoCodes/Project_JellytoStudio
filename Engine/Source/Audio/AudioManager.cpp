#include "Framework.h"
#include "Audio/AudioManager.h"
#include "Audio/AudioClip.h"

#include <FMOD/fmod.hpp>
#include <FMOD/fmod_errors.h>

#define FMOD_CHECK(result)                                              \
    do {                                                                \
        if ((result) != FMOD_OK)                                        \
        {                                                               \
            wchar_t _buf[256];                                          \
            ::swprintf_s(_buf, L"[AudioManager] FMOD Error: %hs\n",    \
                FMOD_ErrorString(result));                              \
            ::OutputDebugStringW(_buf);                                 \
            assert(false);                                              \
        }                                                               \
    } while(false)

struct AudioManager::AudioClipEntry
{
    AudioClip clip;
};

static FMOD_VECTOR ToFmodVec(const Vec3& v)
{
    return { v.x, v.y, v.z };
}

bool AudioManager::Init(const std::wstring& assetRootPath)
{
    FMOD_RESULT result = FMOD::System_Create(&_pSystem);
    FMOD_CHECK(result);

    result = _pSystem->init(kMaxChannels, FMOD_INIT_NORMAL, nullptr);
    FMOD_CHECK(result);

    result = _pSystem->set3DSettings(
        1.0f,
        1.0f,
        1.0f);
    FMOD_CHECK(result);

    for (uint32 i = 0; i < static_cast<uint32>(SoundEvent::Count); ++i)
        _clips[i] = nullptr;

    auto makePath = [&](const wchar_t* file) -> std::wstring
        {
            return assetRootPath + file;
        };

    LoadClip(SoundEvent::BlockPlace, makePath(L"SFX/block_place.wav"), true, false);
    LoadClip(SoundEvent::BlockRemove, makePath(L"SFX/block_remove.wav"), true, false);
    LoadClip(SoundEvent::UIClick, makePath(L"SFX/ui_click.wav"), false, false);
    LoadClip(SoundEvent::BGM_Main, makePath(L"BGM/bgm_main.wav"), false, true);

    _initialized = true;
    return true;
}

bool AudioManager::LoadClip(SoundEvent ev, const std::wstring& filePath,
    bool is3D, bool isLoop)
{
    const uint32 idx = static_cast<uint32>(ev);

    std::string pathA(filePath.begin(), filePath.end());

    FMOD_MODE mode = FMOD_DEFAULT;
    mode |= is3D ? FMOD_3D : FMOD_2D;
    mode |= isLoop ? FMOD_LOOP_NORMAL : FMOD_LOOP_OFF;
    mode |= is3D ? FMOD_3D_LINEARROLLOFF : 0;

    auto* entry = new AudioClipEntry();
    entry->clip.is3D = is3D;
    entry->clip.isLoop = isLoop;

    const FMOD_RESULT result = _pSystem->createSound(
        pathA.c_str(),
        mode,
        nullptr,
        &entry->clip.sound);

    if (result != FMOD_OK)
    {
        wchar_t buf[512];
        ::swprintf_s(buf, L"[AudioManager] Failed to load: %s (%hs)\n",
            filePath.c_str(), FMOD_ErrorString(result));
        ::OutputDebugStringW(buf);
        delete entry;
        return false;
    }

    if (is3D)
    {
        entry->clip.sound->set3DMinMaxDistance(
            kDefault3DMinDist,
            kDefault3DMaxDist);
    }

    _clips[idx] = entry;
    return true;
}

void AudioManager::Update()
{
    if (!_initialized || !_pSystem) return;
    _pSystem->update();
}

void AudioManager::Shutdown()
{
    if (!_pSystem) return;

    StopBGM(true);

    for (auto*& entry : _clips)
    {
        if (entry)
        {
            entry->clip.Release();
            delete entry;
            entry = nullptr;
        }
    }

    _pSystem->close();
    _pSystem->release();
    _pSystem = nullptr;

    _initialized = false;
}

void AudioManager::PlayOneShot2D(SoundEvent ev)
{
    if (!_initialized) return;

    const uint32 idx = static_cast<uint32>(ev);
    if (!_clips[idx] || !_clips[idx]->clip.IsValid()) return;

    FMOD::Channel* channel = nullptr;
    FMOD_RESULT result = _pSystem->playSound(
        _clips[idx]->clip.sound,
        nullptr,
        false,
        &channel);

    if (result != FMOD_OK || !channel) return;

    channel->setVolume(_sfxVolume * _masterVolume);
}

void AudioManager::PlayOneShot3D(SoundEvent ev, const Vec3& worldPos)
{
    if (!_initialized) return;

    const uint32 idx = static_cast<uint32>(ev);
    if (!_clips[idx] || !_clips[idx]->clip.IsValid()) return;

    FMOD::Channel* channel = nullptr;
    FMOD_RESULT result = _pSystem->playSound(
        _clips[idx]->clip.sound,
        nullptr,
        true,
        &channel);

    if (result != FMOD_OK || !channel) return;

    const FMOD_VECTOR pos = ToFmodVec(worldPos);
    const FMOD_VECTOR vel = { 0.f, 0.f, 0.f };
    channel->set3DAttributes(&pos, &vel);
    channel->setVolume(_sfxVolume * _masterVolume);
    channel->setPaused(false);
}

void AudioManager::SetListener(const Vec3& pos, const Vec3& forward, const Vec3& up)
{
    if (!_initialized) return;

    const FMOD_VECTOR fPos = ToFmodVec(pos);
    const FMOD_VECTOR fVel = { 0.f, 0.f, 0.f };
    const FMOD_VECTOR fFwd = ToFmodVec(forward);
    const FMOD_VECTOR fUp = ToFmodVec(up);

    _pSystem->set3DListenerAttributes(0, &fPos, &fVel, &fFwd, &fUp);
}

void AudioManager::SetMasterVolume(float volume)
{
    _masterVolume = std::clamp(volume, 0.f, 1.f);
}

void AudioManager::SetBGMVolume(float volume)
{
    _bgmVolume = std::clamp(volume, 0.f, 1.f);
    if (_pBGMChannel)
        _pBGMChannel->setVolume(_bgmVolume * _masterVolume);
}

void AudioManager::SetSFXVolume(float volume)
{
    _sfxVolume = std::clamp(volume, 0.f, 1.f);
}

void AudioManager::PlayBGM()
{
    if (!_initialized) return;

    const uint32 idx = static_cast<uint32>(SoundEvent::BGM_Main);
    if (!_clips[idx] || !_clips[idx]->clip.IsValid()) return;

    bool isPlaying = false;
    if (_pBGMChannel)
        _pBGMChannel->isPlaying(&isPlaying);

    if (isPlaying) return;

    _pSystem->playSound(
        _clips[idx]->clip.sound,
        nullptr,
        false,
        &_pBGMChannel);

    if (_pBGMChannel)
        _pBGMChannel->setVolume(_bgmVolume * _masterVolume);
}

void AudioManager::StopBGM(bool immediate)
{
    if (!_pBGMChannel) return;

    bool isPlaying = false;
    _pBGMChannel->isPlaying(&isPlaying);
    if (!isPlaying) return;

    if (immediate)
    {
        _pBGMChannel->stop();
    }
    else
    {
        _pBGMChannel->setVolume(0.f);
        _pBGMChannel->stop();
    }

    _pBGMChannel = nullptr;
}

void AudioManager::PauseBGM(bool pause)
{
    if (!_pBGMChannel) return;
    _pBGMChannel->setPaused(pause);
}
