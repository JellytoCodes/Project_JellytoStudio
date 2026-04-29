#include "Framework.h"

#include "Audio/AudioManager.h"
#include "Audio/AudioDataTable.h"

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

static FMOD_VECTOR ToFmodVec(const Vec3& v) { return { v.x, v.y, v.z }; }

bool AudioManager::Init(const std::wstring& audioRootPath)
{
    AudioDataTable* table = GET_SINGLE(AudioDataTable);
    assert(table->IsLoaded() &&
        "AudioManager::Init() 호출 전에 AudioDataTable::Load() 가 완료되어야 합니다.");
    if (!table->IsLoaded()) return false;

    // FMOD 시스템 초기화
    FMOD_RESULT result = FMOD::System_Create(&_pSystem);
    FMOD_CHECK(result);

    result = _pSystem->init(kMaxChannels, FMOD_INIT_NORMAL, nullptr);
    FMOD_CHECK(result);

    result = _pSystem->set3DSettings(1.0f, 1.0f, 1.0f);
    FMOD_CHECK(result);

    // 해시맵 예약 (로드 시점에만 발생하는 1회성 할당)
    _clips.reserve(table->GetAllRecords().size());

    // Data-Driven 로드
    for (const AudioClipRecord& rec : table->GetAllRecords())
    {
        LoadClip(rec.key, audioRootPath + rec.path,
            rec.is3D, rec.isLoop, rec.minDist, rec.maxDist);
    }

    _initialized = true;
    return true;
}

AudioManager::AudioClipEntry* AudioManager::FindClip(const std::unordered_map<std::wstring, AudioManager::AudioClipEntry*>& clips, const std::wstring& key)
{
    const auto it = clips.find(key);
    if (it == clips.end() || !it->second || !it->second->clip.IsValid())
    {
        wchar_t buf[256];
        ::swprintf_s(buf, L"[AudioManager] 클립을 찾을 수 없습니다: '%s'\n", key.c_str());
        ::OutputDebugStringW(buf);
        return nullptr;
    }
    return it->second;
}

bool AudioManager::LoadClip(const std::wstring& key, const std::wstring& fullPath,
    bool is3D, bool isLoop, float minDist, float maxDist)
{
    std::string pathA(fullPath.begin(), fullPath.end());

    FMOD_MODE mode = FMOD_DEFAULT;
    mode |= is3D ? FMOD_3D : FMOD_2D;
    mode |= isLoop ? FMOD_LOOP_NORMAL : FMOD_LOOP_OFF;
    mode |= is3D ? FMOD_3D_LINEARROLLOFF : 0;

    auto* entry = new AudioClipEntry();
    entry->clip.is3D = is3D;
    entry->clip.isLoop = isLoop;

    const FMOD_RESULT result = _pSystem->createSound(
        pathA.c_str(), mode, nullptr, &entry->clip.sound);

    if (result != FMOD_OK)
    {
        wchar_t buf[512];
        ::swprintf_s(buf, L"[AudioManager] Failed to load '%s': %hs\n",
            fullPath.c_str(), FMOD_ErrorString(result));
        ::OutputDebugStringW(buf);
        delete entry;
        return false;
    }

    if (is3D)
        entry->clip.sound->set3DMinMaxDistance(minDist, maxDist);

    _clips.emplace(key, entry);
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

    for (auto& [key, entry] : _clips)
    {
        if (entry)
        {
            entry->clip.Release();
            delete entry;
        }
    }
    _clips.clear();

    _pSystem->close();
    _pSystem->release();
    _pSystem = nullptr;
    _initialized = false;
}

void AudioManager::PlayOneShot2D(const std::wstring& key)
{
    if (!_initialized) return;

    auto* entry = FindClip(_clips, key);
    if (!entry) return;

    FMOD::Channel* channel = nullptr;
    if (_pSystem->playSound(entry->clip.sound, nullptr, false, &channel) != FMOD_OK
        || !channel) return;

    channel->setVolume(_sfxVolume * _masterVolume);
}

void AudioManager::PlayOneShot3D(const std::wstring& key, const Vec3& worldPos)
{
    if (!_initialized) return;

    auto* entry = FindClip(_clips, key);
    if (!entry) return;

    FMOD::Channel* channel = nullptr;
    if (_pSystem->playSound(entry->clip.sound, nullptr, true, &channel) != FMOD_OK
        || !channel) return;

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

void AudioManager::PlayBGM(const std::wstring& key)
{
    if (!_initialized) return;

    auto* entry = FindClip(_clips, key);
    if (!entry) return;

    bool isPlaying = false;
    if (_pBGMChannel)
        _pBGMChannel->isPlaying(&isPlaying);
    if (isPlaying) return;

    _pSystem->playSound(entry->clip.sound, nullptr, false, &_pBGMChannel);
    if (_pBGMChannel)
        _pBGMChannel->setVolume(_bgmVolume * _masterVolume);
}

void AudioManager::StopBGM(bool immediate)
{
    if (!_pBGMChannel) return;

    bool isPlaying = false;
    _pBGMChannel->isPlaying(&isPlaying);
    if (!isPlaying) return;

    if (!immediate)
        _pBGMChannel->setVolume(0.f);

    _pBGMChannel->stop();
    _pBGMChannel = nullptr;
}

void AudioManager::PauseBGM(bool pause)
{
    if (_pBGMChannel)
        _pBGMChannel->setPaused(pause);
}