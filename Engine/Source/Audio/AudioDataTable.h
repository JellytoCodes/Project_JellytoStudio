#pragma once

struct AudioClipRecord
{
    std::wstring key;            
    std::wstring path;           
    bool         is3D   = false;
    bool         isLoop = false;
    float        minDist = 1.0f; 
    float        maxDist = 30.0f;
};

class AudioDataTable
{
    DECLARE_SINGLE(AudioDataTable)

public:
    void Load(const std::wstring& jsonPath);

    const AudioClipRecord* GetRecord(const std::wstring& key) const;

    const std::vector<AudioClipRecord>& GetAllRecords() const { return m_records; }
    bool IsLoaded() const { return m_loaded; }

private:

    std::vector<AudioClipRecord>                             m_records;
    std::unordered_map<std::wstring, const AudioClipRecord*> m_keyMap;
    bool                                                     m_loaded = false;
};