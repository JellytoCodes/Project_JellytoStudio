
#include "Framework.h"
#include "Audio/AudioDataTable.h"

std::wstring AudioDataTable::Utf8ToWide(const char* utf8)
{
    if (!utf8 || utf8[0] == '\0') return {};
    const int wlen = ::MultiByteToWideChar(CP_UTF8, 0, utf8, -1, nullptr, 0);
    if (wlen <= 1) return {};
    std::wstring result(static_cast<size_t>(wlen - 1), L'\0');
    ::MultiByteToWideChar(CP_UTF8, 0, utf8, -1, result.data(), wlen);
    return result;
}

bool AudioDataTable::AttrBool(const tinyxml2::XMLElement* e, const char* name, bool def)
{
    const char* val = e->Attribute(name);
    if (!val) return def;
    return (std::string_view(val) == "true");
}

float AudioDataTable::AttrFloat(const tinyxml2::XMLElement* e, const char* name, float def)
{
    float v = def;
    e->QueryFloatAttribute(name, &v);
    return v;
}

void AudioDataTable::Load(const std::wstring& xmlPath)
{
    assert(!m_loaded && "AudioDataTable::Load() 는 앱 시작 시 1회만 호출해야 합니다.");

    const int narrowLen = ::WideCharToMultiByte(
        CP_UTF8, 0, xmlPath.c_str(), -1, nullptr, 0, nullptr, nullptr);
    std::string narrowPath(static_cast<size_t>(narrowLen - 1), '\0');
    ::WideCharToMultiByte(
        CP_UTF8, 0, xmlPath.c_str(), -1, narrowPath.data(), narrowLen, nullptr, nullptr);

    tinyxml2::XMLDocument doc;
    const tinyxml2::XMLError err = doc.LoadFile(narrowPath.c_str());
    assert(err == tinyxml2::XML_SUCCESS &&
        "AudioData.xml 파일을 찾을 수 없거나 파싱에 실패했습니다.");
    if (err != tinyxml2::XML_SUCCESS) return;

    const tinyxml2::XMLElement* root = doc.RootElement();
    assert(root && "<AudioDataTable> 루트 요소가 없습니다.");

    const tinyxml2::XMLElement* clipsElem = root->FirstChildElement("Clips");
    assert(clipsElem && "<Clips> 요소가 없습니다. AudioData.xml 을 확인하세요.");

    int32 count = 0;
    for (const auto* e = clipsElem->FirstChildElement("Clip");
        e; e = e->NextSiblingElement("Clip"))
        ++count;

    m_records.reserve(static_cast<size_t>(count));

    for (const auto* e = clipsElem->FirstChildElement("Clip");
        e; e = e->NextSiblingElement("Clip"))
    {
        AudioClipRecord rec;
        rec.key = Utf8ToWide(e->Attribute("key"));
        rec.path = Utf8ToWide(e->Attribute("path"));
        rec.is3D = AttrBool(e, "is3D", false);
        rec.isLoop = AttrBool(e, "isLoop", false);
        rec.minDist = AttrFloat(e, "minDist", 1.0f);
        rec.maxDist = AttrFloat(e, "maxDist", 30.0f);

        assert(!rec.key.empty() && "AudioData.xml <Clip> 의 key 가 비어 있습니다.");
        assert(!rec.path.empty() && "AudioData.xml <Clip> 의 path 가 비어 있습니다.");

        m_records.push_back(std::move(rec));
    }

    m_keyMap.reserve(m_records.size());
    for (const auto& rec : m_records)
    {
        const auto [it, inserted] = m_keyMap.emplace(rec.key, &rec);
        assert(inserted && "AudioData.xml 에 중복된 key 가 존재합니다.");
    }

    m_loaded = true;
}

const AudioClipRecord* AudioDataTable::GetRecord(const std::wstring& key) const
{
    const auto it = m_keyMap.find(key);
    return (it != m_keyMap.end()) ? it->second : nullptr;
}