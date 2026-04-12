#include "pch.h"
#include "BlockDataTable.h"

using SlotType = PaletteWidget::SlotType;

std::wstring BlockDataTable::Utf8ToWide(const char* utf8)
{
    if (!utf8 || utf8[0] == '\0') return {};

    const int wlen = ::MultiByteToWideChar(CP_UTF8, 0, utf8, -1, nullptr, 0);
    if (wlen <= 1) return {};                  // 변환 길이 = 1 은 NUL 만 있는 경우

    std::wstring result(static_cast<size_t>(wlen - 1), L'\0');
    ::MultiByteToWideChar(CP_UTF8, 0, utf8, -1, result.data(), wlen);
    return result;
}

float BlockDataTable::AttrFloat(const tinyxml2::XMLElement* elem, const char* name, float def)
{
    float v = def;
    elem->QueryFloatAttribute(name, &v);
    return v;
}

void BlockDataTable::Load(const std::wstring& xmlPath)
{
    assert(!m_loaded && "BlockDataTable::Load() 는 앱 시작 시 1회만 호출해야 합니다.");

    const int narrowLen = ::WideCharToMultiByte(
        CP_UTF8, 0, xmlPath.c_str(), -1, nullptr, 0, nullptr, nullptr);
    std::string narrowPath(static_cast<size_t>(narrowLen - 1), '\0');
    ::WideCharToMultiByte(
        CP_UTF8, 0, xmlPath.c_str(), -1, narrowPath.data(), narrowLen, nullptr, nullptr);

    tinyxml2::XMLDocument doc;
    const tinyxml2::XMLError err = doc.LoadFile(narrowPath.c_str());
    assert(err == tinyxml2::XML_SUCCESS &&
           "BlockData.xml 파일을 찾을 수 없거나 파싱에 실패했습니다.");
    if (err != tinyxml2::XML_SUCCESS) return;

    const tinyxml2::XMLElement* root = doc.RootElement();
    assert(root && "<BlockDataTable> 루트 요소가 없습니다.");

    const tinyxml2::XMLElement* slotsElem = root->FirstChildElement("Slots");
    assert(slotsElem && "<Slots> 요소가 없습니다. BlockData.xml 을 확인하세요.");

    int32 slotCount = 0;
    for (const auto* e = slotsElem->FirstChildElement("Slot");
         e != nullptr;
         e = e->NextSiblingElement("Slot"))
    {
        ++slotCount;
    }
    m_slotRecords.reserve(static_cast<size_t>(slotCount));

    for (const auto* e = slotsElem->FirstChildElement("Slot");
         e != nullptr;
         e = e->NextSiblingElement("Slot"))
    {
        int id = 0;
        e->QueryIntAttribute("id", &id);

        BlockSlotRecord rec;
        rec.slotType     = static_cast<SlotType>(id);
        rec.key          = Utf8ToWide(e->Attribute("key"));
        rec.label        = Utf8ToWide(e->Attribute("label"));
        rec.paletteLabel = Utf8ToWide(e->Attribute("paletteLabel"));
        rec.modelName    = Utf8ToWide(e->Attribute("modelName"));
        rec.color        = Color(
            AttrFloat(e, "colorR"),
            AttrFloat(e, "colorG"),
            AttrFloat(e, "colorB"),
            AttrFloat(e, "colorA", 1.f)
        );
        //rec.isEraser = (std::string(e->Attribute("isEraser", "false")) == "true");

        m_slotRecords.push_back(std::move(rec));
    }

    m_keyMap.reserve(m_slotRecords.size());
    for (const auto& rec : m_slotRecords)
        m_keyMap.emplace(rec.key, &rec);

    const tinyxml2::XMLElement* phaseSeqElem = root->FirstChildElement("PhaseSequence");
    assert(phaseSeqElem && "<PhaseSequence> 요소가 없습니다. BlockData.xml 을 확인하세요.");

    int32 phaseCount = 0;
    for (const auto* e = phaseSeqElem->FirstChildElement("Phase");
         e != nullptr;
         e = e->NextSiblingElement("Phase"))
    {
        ++phaseCount;
    }
    m_phaseRecords.reserve(static_cast<size_t>(phaseCount));

    for (const auto* e = phaseSeqElem->FirstChildElement("Phase");
         e != nullptr;
         e = e->NextSiblingElement("Phase"))
    {
        const std::wstring slotKey  = Utf8ToWide(e->Attribute("slotKey"));
        const BlockSlotRecord* slotRec = GetSlotRecordByKey(slotKey);

        assert(slotRec != nullptr &&
               "PhaseSequence 의 slotKey 가 Slots 섹션에 존재하지 않습니다.");
        if (!slotRec) continue;

        PhaseRecord phase;
        phase.dropSlot     = slotRec->slotType;
        phase.modelName    = slotRec->modelName;
        phase.phaseName    = Utf8ToWide(e->Attribute("phaseName"));
        phase.breaksToNext = 0;
        e->QueryIntAttribute("breaksToNext", &phase.breaksToNext);

        m_phaseRecords.push_back(std::move(phase));
    }

    m_loaded = true;
}

const BlockSlotRecord* BlockDataTable::GetSlotRecord(SlotType type) const
{
    const int32 idx = static_cast<int32>(type);
    if (idx < 0 || idx >= static_cast<int32>(m_slotRecords.size())) return nullptr;
    return &m_slotRecords[static_cast<size_t>(idx)];
}

const BlockSlotRecord* BlockDataTable::GetSlotRecordByKey(const std::wstring& key) const
{
    const auto it = m_keyMap.find(key);
    return (it != m_keyMap.end()) ? it->second : nullptr;
}