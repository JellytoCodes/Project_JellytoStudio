#include "pch.h"
#include "BlockTable.h"
#include "Utils/tinyxml2.h"

using SlotType = PaletteWidget::SlotType;
using CH       = CollisionChannel;
using PF       = PlaceFace;

std::wstring BlockTable::Utf8ToWide(const char* utf8)
{
    if (!utf8 || utf8[0] == '\0') return {};
    const int n = ::MultiByteToWideChar(CP_UTF8, 0, utf8, -1, nullptr, 0);
    if (n <= 1) return {};
    std::wstring w(static_cast<size_t>(n - 1), L'\0');
    ::MultiByteToWideChar(CP_UTF8, 0, utf8, -1, w.data(), n);
    return w;
}

std::string BlockTable::WideToUtf8(const std::wstring& w)
{
    if (w.empty()) return {};
    const int n = ::WideCharToMultiByte(CP_UTF8, 0, w.c_str(), -1, nullptr, 0, nullptr, nullptr);
    std::string s(static_cast<size_t>(n - 1), '\0');
    ::WideCharToMultiByte(CP_UTF8, 0, w.c_str(), -1, s.data(), n, nullptr, nullptr);
    return s;
}

float BlockTable::AttrFloat(const tinyxml2::XMLElement* e, const char* name, float def)
{
    float v = def;
    if (e) e->QueryFloatAttribute(name, &v);
    return v;
}

ColliderSize BlockTable::ParseCollider(const char* s)
{
    if (!s) return ColliderSize::Unit;
    const std::string str(s);
    if (str == "Small") return ColliderSize::Small;
    if (str == "Tall")  return ColliderSize::Tall;
    if (str == "Wide")  return ColliderSize::Wide;
    return ColliderSize::Unit;
}

CollisionChannel BlockTable::ParseChannel(const char* s)
{
    if (!s) return CH::Default;
    const std::string str(s);
    if (str == "Priming")   return CH::Priming;
    if (str == "Floor")     return CH::Floor;
    if (str == "Mushroom")  return CH::Mushroom;
    if (str == "Character") return CH::Character;
    return CH::Default;
}

uint8 BlockTable::ParsePickable(const char* s)
{
    if (!s) return 0xFF;
    const std::string str(s);
    if (str == "All") return 0xFF;
    uint8 mask = 0;
    if (str.find("Priming")   != std::string::npos) mask |= static_cast<uint8>(CH::Priming);
    if (str.find("Floor")     != std::string::npos) mask |= static_cast<uint8>(CH::Floor);
    if (str.find("Mushroom")  != std::string::npos) mask |= static_cast<uint8>(CH::Mushroom);
    if (str.find("Character") != std::string::npos) mask |= static_cast<uint8>(CH::Character);
    return mask ? mask : 0xFF;
}

uint8 BlockTable::ParseFaces(const char* s)
{
    if (!s) return 0xFF;
    const std::string str(s);
    if (str == "All") return 0xFF;
    uint8 mask = 0;
    if (str.find("Top")  != std::string::npos) mask |= static_cast<uint8>(PF::Top);
    if (str.find("Side") != std::string::npos) mask |= static_cast<uint8>(PF::Side);
    if (str.find("Bot")  != std::string::npos) mask |= static_cast<uint8>(PF::Bottom);
    return mask ? mask : 0xFF;
}

BlockRenderType BlockTable::ParseRenderType(const char* s)
{
    if (s && std::string(s) == "Model") return BlockRenderType::Model;
    return BlockRenderType::Mesh;
}

void BlockTable::Load(const std::wstring& xmlPath)
{
    assert(!_loaded && "BlockTable::Load() 는 앱 시작 시 1회만 호출해야 합니다.");

    tinyxml2::XMLDocument doc;
    const auto err = doc.LoadFile(WideToUtf8(xmlPath).c_str());
    assert(err == tinyxml2::XML_SUCCESS && "BlockMaster.xml 로드 실패");
    if (err != tinyxml2::XML_SUCCESS) return;

    const auto* root = doc.RootElement();
    assert(root);

    if (const char* pal = root->Attribute("paletteTex"))
        _palettePath = Utf8ToWide(pal);
    if (_palettePath.empty())
        _palettePath = L"../Resources/Textures/MapModel/Main_texture.png";

    const auto* blocksElem = root->FirstChildElement("Blocks");
    assert(blocksElem && "<Blocks> 요소 없음");

    for (const auto* e = blocksElem->FirstChildElement("Block");
         e; e = e->NextSiblingElement("Block"))
    {
        BlockRecord rec;

        e->QueryIntAttribute("id", &rec.typeId);

        if (const char* v = e->Attribute("key"))          rec.key          = Utf8ToWide(v);
        if (const char* v = e->Attribute("label"))        rec.label        = Utf8ToWide(v);
        if (const char* v = e->Attribute("paletteLabel")) rec.paletteLabel = Utf8ToWide(v);

        rec.color    = Color(AttrFloat(e,"colorR"), AttrFloat(e,"colorG"),
                             AttrFloat(e,"colorB"), AttrFloat(e,"colorA", 1.f));
        //rec.isEraser = (std::string(e->Attribute("isEraser","false")) == "true");

        rec.renderType = ParseRenderType(e->Attribute("renderType"));

        if (rec.renderType == BlockRenderType::Model)
        {
            if (const char* v = e->Attribute("modelName")) rec.modelName = Utf8ToWide(v);
            e->QueryFloatAttribute("modelScale", &rec.modelScale);
            rec.paletteRect = { 0.f, 0.f, 1.f, 1.f };
        }
        else
        {
            e->QueryFloatAttribute("paletteU", &rec.paletteRect.uOffset);
            e->QueryFloatAttribute("paletteV", &rec.paletteRect.vOffset);
            rec.paletteRect.uScale = 0.f;
            rec.paletteRect.vScale = 0.f;
        }

        if (const char* v = e->Attribute("collider"))   rec.collider    = ParseCollider(v);
        if (const char* v = e->Attribute("ownChannel")) rec.ownChannel  = ParseChannel(v);
        if (const char* v = e->Attribute("pickable"))   rec.pickableMask = ParsePickable(v);
        if (const char* v = e->Attribute("faces"))      rec.faceMask    = ParseFaces(v);

        const int32 slot = rec.typeId;
        if (slot >= static_cast<int32>(_records.size()))
        {
            _records.resize(slot + 1);
            _uvRects.resize(slot + 1, BlockUVRect{});
        }
        _records[slot]  = rec;
        _uvRects[slot]  = rec.paletteRect;
    }

    _keyMap.reserve(_records.size());
    for (const auto& rec : _records)
        if (!rec.key.empty()) _keyMap.emplace(rec.key, &rec);

    const auto* phaseElem = root->FirstChildElement("PhaseSequence");
    assert(phaseElem && "<PhaseSequence> 요소 없음");

    for (const auto* e = phaseElem->FirstChildElement("Phase");
         e; e = e->NextSiblingElement("Phase"))
    {
        const std::wstring slotKey = Utf8ToWide(e->Attribute("slotKey"));
        const BlockRecord* slotRec = GetRecordByKey(slotKey);
        assert(slotRec && "PhaseSequence slotKey 가 Blocks 에 없음");
        if (!slotRec) continue;

        PhaseRecord phase;
        phase.dropSlot     = static_cast<SlotType>(slotRec->typeId);
        phase.modelName    = slotRec->modelName;
        phase.phaseName    = Utf8ToWide(e->Attribute("phaseName"));
        phase.breaksToNext = 0;
        e->QueryIntAttribute("breaksToNext", &phase.breaksToNext);
        _phases.push_back(std::move(phase));
    }

    _loaded = true;
}

const BlockRecord* BlockTable::GetRecord(int32 typeId) const
{
    if (typeId < 0 || typeId >= static_cast<int32>(_records.size())) return nullptr;
    return &_records[typeId];
}

const BlockRecord* BlockTable::GetRecord(SlotType st) const
{
    return GetRecord(static_cast<int32>(st));
}

const BlockRecord* BlockTable::GetRecordByKey(const std::wstring& key) const
{
    const auto it = _keyMap.find(key);
    return (it != _keyMap.end()) ? it->second : nullptr;
}