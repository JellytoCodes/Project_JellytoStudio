#include "Framework.h"
#include "Audio/AudioDataTable.h"

using json = nlohmann::json;

namespace
{
    std::wstring Utf8ToWide(const std::string& utf8)
    {
        if (utf8.empty()) return {};
        const int wlen = ::MultiByteToWideChar(
            CP_UTF8, 0, utf8.c_str(), static_cast<int>(utf8.size()), nullptr, 0);
        if (wlen <= 0) return {};
        std::wstring result(static_cast<size_t>(wlen), L'\0');
        ::MultiByteToWideChar(
            CP_UTF8, 0, utf8.c_str(), static_cast<int>(utf8.size()),
            result.data(), wlen);
        return result;
    }

    std::string WideToUtf8(const std::wstring& w)
    {
        if (w.empty()) return {};
        const int n = ::WideCharToMultiByte(
            CP_UTF8, 0, w.c_str(), -1, nullptr, 0, nullptr, nullptr);
        std::string s(static_cast<size_t>(n - 1), '\0');
        ::WideCharToMultiByte(CP_UTF8, 0, w.c_str(), -1, s.data(), n, nullptr, nullptr);
        return s;
    }

    template<typename T>
    T GetOrDefault(const json& j, const char* key, T def)
    {
        if (j.contains(key) && !j[key].is_null())
            return j[key].get<T>();
        return def;
    }
}

void AudioDataTable::Load(const std::wstring& jsonPath)
{
    assert(!m_loaded && "AudioDataTable::Load() 는 앱 시작 시 1회만 호출해야 합니다.");

    std::ifstream file(WideToUtf8(jsonPath));
    assert(file.is_open() && "AudioData.json 파일을 찾을 수 없습니다.");
    if (!file.is_open()) return;

    json doc;
    try
    {
        file >> doc;
    }
    catch (const json::parse_error& e)
    {
        assert(false && "AudioData.json 파싱 실패 — JSON 문법 오류");
        ::OutputDebugStringA(e.what());
        return;
    }

    assert(doc.contains("clips") && "AudioData.json 에 \"clips\" 키가 없습니다.");
    const auto& clips = doc["clips"];
    m_records.reserve(clips.size());

    for (const auto& c : clips)
    {
        if (!c.contains("key")) continue;

        AudioClipRecord rec;
        rec.key     = Utf8ToWide(c["key"].get<std::string>());
        rec.path    = Utf8ToWide(c["path"].get<std::string>());
        rec.is3D    = GetOrDefault(c, "is3D",    false);
        rec.isLoop  = GetOrDefault(c, "isLoop",  false);
        rec.minDist = GetOrDefault(c, "minDist", 1.0f);
        rec.maxDist = GetOrDefault(c, "maxDist", 30.0f);

        assert(!rec.key.empty()  && "AudioData.json <clip> 의 key 가 비어 있습니다.");
        assert(!rec.path.empty() && "AudioData.json <clip> 의 path 가 비어 있습니다.");

        m_records.push_back(std::move(rec));
    }

    m_keyMap.reserve(m_records.size());
    for (const auto& rec : m_records)
    {
        const auto [it, inserted] = m_keyMap.emplace(rec.key, &rec);
        assert(inserted && "AudioData.json 에 중복된 key 가 존재합니다.");
    }

    m_loaded = true;
}

const AudioClipRecord* AudioDataTable::GetRecord(const std::wstring& key) const
{
    const auto it = m_keyMap.find(key);
    return (it != m_keyMap.end()) ? it->second : nullptr;
}