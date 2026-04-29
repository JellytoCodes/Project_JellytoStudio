#pragma once

namespace tinyxml2 { class XMLElement; }

struct AudioClipRecord
{
    std::wstring key;             
    std::wstring path;            
    bool         is3D = false;   
    bool         isLoop = false;  
    float        minDist = 1.0f;  
    float        maxDist = 30.0f; 
};

class AudioDataTable
{
    DECLARE_SINGLE(AudioDataTable)

public:
    void Load(const std::wstring& xmlPath);

    const AudioClipRecord* GetRecord(const std::wstring& key) const;

    const std::vector<AudioClipRecord>& GetAllRecords() const { return m_records; }
    bool IsLoaded() const { return m_loaded; }

private:
    static std::wstring Utf8ToWide(const char* utf8);
    static bool         AttrBool(const tinyxml2::XMLElement* e, const char* name, bool  def = false);
    static float        AttrFloat(const tinyxml2::XMLElement* e, const char* name, float def = 0.f);

    std::vector<AudioClipRecord>                             m_records;  // 삽입 순서 보존
    std::unordered_map<std::wstring, const AudioClipRecord*> m_keyMap;   // O(1) 조회
    bool                                                     m_loaded = false;
};