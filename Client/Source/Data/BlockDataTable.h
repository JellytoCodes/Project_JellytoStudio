#pragma once

#include "UI/PaletteWidget.h"

namespace tinyxml2 { class XMLElement; }

struct BlockSlotRecord
{
    std::wstring             key;         
    std::wstring             label;       
    std::wstring             paletteLabel;
    std::wstring             modelName;   
    Color                    color;       
    PaletteWidget::SlotType  slotType;    
    bool                     isEraser = false;
};

struct PhaseRecord
{
    PaletteWidget::SlotType  dropSlot;    
    std::wstring             modelName;   
    std::wstring             phaseName;   
    int32                    breaksToNext;
};

class BlockDataTable
{
    DECLARE_SINGLE(BlockDataTable)
public:

    ~BlockDataTable() = default;
    BlockDataTable(const BlockDataTable&)            = delete;
    BlockDataTable& operator=(const BlockDataTable&) = delete;

    void Load(const std::wstring& xmlPath);
    const BlockSlotRecord* GetSlotRecord(PaletteWidget::SlotType type) const;
    const BlockSlotRecord* GetSlotRecordByKey(const std::wstring& key) const;
    const std::vector<BlockSlotRecord>& GetAllSlotRecords() const { return m_slotRecords; }
    const std::vector<PhaseRecord>& GetPhaseSequence() const { return m_phaseRecords; }
    bool IsLoaded() const { return m_loaded; }

private:
    static std::wstring Utf8ToWide(const char* utf8);

    static float AttrFloat(const tinyxml2::XMLElement* elem, const char* name, float def = 0.f);

    std::vector<BlockSlotRecord>                              m_slotRecords;
    std::unordered_map<std::wstring, const BlockSlotRecord*>  m_keyMap;
    std::vector<PhaseRecord>                                  m_phaseRecords;
    bool                                                      m_loaded = false;
};