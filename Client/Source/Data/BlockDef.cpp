#include "pch.h"
#include "BlockDef.h"
#include "BlockDataTable.h"
#include "UI/PaletteWidget.h"
#include "Utils/tinyxml2.h"

using CH = CollisionChannel;
using PF = PlaceFace;
using SlotType = PaletteWidget::SlotType;

static std::string WStrToStr(const std::wstring& w)
{
    if (w.empty()) return {};
    int n = ::WideCharToMultiByte(CP_UTF8, 0, w.c_str(), -1, nullptr, 0, nullptr, nullptr);
    std::string s(n - 1, 0);
    ::WideCharToMultiByte(CP_UTF8, 0, w.c_str(), -1, s.data(), n, nullptr, nullptr);
    return s;
}

static std::wstring StrToWStr(const std::string& s)
{
    if (s.empty()) return {};
    int n = ::MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
    std::wstring w(n - 1, 0);
    ::MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, w.data(), n);
    return w;
}

// ---------------------------------------------------------------------------
// BlockDef.h 의 DECLARE_SINGLE 패턴 초기화
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// PopulateFromDataTable
//   BlockData.xml 이 로드된 BlockDataTable 을 기반으로 BlockDef 를 구성한다.
//   BlockDefs.xml 이 없거나 로드에 실패했을 때 자동으로 호출된다.
//   각 슬롯의 modelName 이 비어있으면 Mesh(큐브) 타입, 있으면 Model(FBX) 타입.
// ---------------------------------------------------------------------------
void BlockDefRegistry::PopulateFromDataTable()
{
    const int32 slotCount = static_cast<int32>(SlotType::Count);
    _defs.assign(slotCount, BlockDef{});
    _uvRects.assign(slotCount, BlockUVRect{});

    auto* table = GET_SINGLE(BlockDataTable);

    for (int32 i = 0; i < slotCount; ++i)
    {
        const SlotType st  = static_cast<SlotType>(i);
        const BlockSlotRecord* rec = table->GetSlotRecord(st);

        BlockDef& def  = _defs[i];
        def.typeId     = i;

        // 이름은 DataTable 에서, 없으면 슬롯 번호로 대체
        def.name = rec ? rec->label : std::wstring(L"Slot") + std::to_wstring(i);

        // 지우개는 별도 처리
        if (st == SlotType::Eraser)
        {
            def.renderType   = BlockRenderType::Mesh;
            def.collider     = ColliderSize::Unit;
            def.ownChannel   = CH::Default;
            def.pickableMask = static_cast<uint8>(CH::All);
            def.faceMask     = static_cast<uint8>(PF::All);
            _uvRects[i]      = def.paletteRect;
            continue;
        }

        const std::wstring modelName = rec ? rec->modelName : L"";

        if (modelName.empty())
        {
            // Case A: 팔레트 단색 큐브
            def.renderType   = BlockRenderType::Mesh;
            def.paletteRect  = { 0.f, 0.f, 0.f, 0.f };
            def.collider     = ColliderSize::Unit;
            def.ownChannel   = CH::Priming;
            def.pickableMask = static_cast<uint8>(CH::Priming)
                             | static_cast<uint8>(CH::Character);
            def.faceMask     = static_cast<uint8>(PF::All);
        }
        else
        {
            // Case B: FBX 모델 (팔레트 UV 베이크)
            def.renderType   = BlockRenderType::Model;
            def.modelName    = modelName;
            def.modelScale   = 0.01f;
            def.paletteRect  = { 0.f, 0.f, 1.f, 1.f };

            // Mushroom 계열 — 슬롯 0~2
            if (i <= 2)
            {
                def.collider     = ColliderSize::Small;
                def.ownChannel   = CH::Mushroom;
                def.pickableMask = static_cast<uint8>(CH::Priming);
                def.faceMask     = static_cast<uint8>(PF::Top);
            }
            else
            {
                // Priming FBX 블록 (잔디 등)
                def.collider     = ColliderSize::Unit;
                def.ownChannel   = CH::Priming;
                def.pickableMask = static_cast<uint8>(CH::Priming)
                                 | static_cast<uint8>(CH::Character);
                def.faceMask     = static_cast<uint8>(PF::All);
            }
        }

        _uvRects[i] = def.paletteRect;
    }

    // 팔레트 텍스처 기본 경로
    if (_palettePath.empty())
        _palettePath = L"../Resources/Textures/MapModel/Main_texture.png";
}

// ---------------------------------------------------------------------------
// Load
//   xmlPath 에서 BlockDefs.xml 을 읽는다.
//   파일이 없거나 파싱 실패 시 PopulateFromDataTable() 로 폴백한다.
// ---------------------------------------------------------------------------
bool BlockDefRegistry::Load(const std::wstring& xmlPath)
{
    tinyxml2::XMLDocument doc;
    if (doc.LoadFile(WStrToStr(xmlPath).c_str()) != tinyxml2::XML_SUCCESS)
    {
        ::OutputDebugStringW((L"[BlockDefRegistry] XML 없음 → DataTable 폴백: " + xmlPath + L"\n").c_str());
        PopulateFromDataTable();
        return false;
    }

    tinyxml2::XMLElement* root = doc.FirstChildElement("BlockDefs");
    if (!root)
    {
        PopulateFromDataTable();
        return false;
    }

    if (const char* pal = root->Attribute("paletteTex"))
        _palettePath = StrToWStr(pal);

    _defs.clear();

    for (auto* elem = root->FirstChildElement("Block"); elem; elem = elem->NextSiblingElement("Block"))
    {
        BlockDef def;
        elem->QueryIntAttribute("id", &def.typeId);

        if (const char* n = elem->Attribute("name"))
            def.name = StrToWStr(n);
        if (const char* rt = elem->Attribute("renderType"))
            def.renderType = (std::string(rt) == "Model") ? BlockRenderType::Model : BlockRenderType::Mesh;

        if (def.renderType == BlockRenderType::Mesh)
        {
            elem->QueryFloatAttribute("paletteU", &def.paletteRect.uOffset);
            elem->QueryFloatAttribute("paletteV", &def.paletteRect.vOffset);
            def.paletteRect.uScale = 0.f;
            def.paletteRect.vScale = 0.f;
        }
        else
        {
            if (const char* mn = elem->Attribute("modelName"))
                def.modelName = StrToWStr(mn);
            elem->QueryFloatAttribute("modelScale", &def.modelScale);
            def.paletteRect = { 0.f, 0.f, 1.f, 1.f };
        }

        if (const char* c = elem->Attribute("collider"))   def.collider     = ParseCollider(c);
        if (const char* c = elem->Attribute("ownChannel")) def.ownChannel   = ParseChannel(c);
        if (const char* p = elem->Attribute("pickable"))   def.pickableMask = ParsePickable(p);
        if (const char* f = elem->Attribute("faces"))      def.faceMask     = ParseFaces(f);

        const int32 slot = def.typeId;
        if (slot >= static_cast<int32>(_defs.size()))
        {
            _defs.resize(slot + 1);
            _uvRects.resize(slot + 1, BlockUVRect{});
        }
        _defs[slot]    = def;
        _uvRects[slot] = def.paletteRect;
    }

    // XML 에서 일부 슬롯이 누락됐으면 DataTable 로 채움
    const int32 expected = static_cast<int32>(SlotType::Count);
    if (static_cast<int32>(_defs.size()) < expected)
    {
        ::OutputDebugStringW(L"[BlockDefRegistry] XML 슬롯 부족 → 나머지를 DataTable 로 채움\n");
        PopulateFromDataTable();
    }

    if (_palettePath.empty())
        _palettePath = L"../Resources/Textures/MapModel/Main_texture.png";

    return true;
}

const BlockDef* BlockDefRegistry::GetDef(int32 typeId) const
{
    if (typeId < 0 || typeId >= static_cast<int32>(_defs.size())) return nullptr;
    return &_defs[typeId];
}

ColliderSize BlockDefRegistry::ParseCollider(const char* s)
{
    if (!s) return ColliderSize::Unit;
    std::string str(s);
    if (str == "Small") return ColliderSize::Small;
    if (str == "Tall")  return ColliderSize::Tall;
    if (str == "Wide")  return ColliderSize::Wide;
    return ColliderSize::Unit;
}

CollisionChannel BlockDefRegistry::ParseChannel(const char* s)
{
    if (!s) return CH::Default;
    std::string str(s);
    if (str == "Priming")   return CH::Priming;
    if (str == "Floor")     return CH::Floor;
    if (str == "Mushroom")  return CH::Mushroom;
    if (str == "Character") return CH::Character;
    return CH::Default;
}

uint8 BlockDefRegistry::ParsePickable(const char* s)
{
    if (!s) return 0xFF;
    uint8 mask = 0;
    std::string str(s);
    if (str.find("Priming")   != std::string::npos) mask |= static_cast<uint8>(CH::Priming);
    if (str.find("Floor")     != std::string::npos) mask |= static_cast<uint8>(CH::Floor);
    if (str.find("Mushroom")  != std::string::npos) mask |= static_cast<uint8>(CH::Mushroom);
    if (str.find("Character") != std::string::npos) mask |= static_cast<uint8>(CH::Character);
    if (str == "All")                               mask  = 0xFF;
    return mask;
}

uint8 BlockDefRegistry::ParseFaces(const char* s)
{
    if (!s) return 0xFF;
    uint8 mask = 0;
    std::string str(s);
    if (str.find("Top")  != std::string::npos) mask |= static_cast<uint8>(PF::Top);
    if (str.find("Side") != std::string::npos) mask |= static_cast<uint8>(PF::Side);
    if (str.find("Bot")  != std::string::npos) mask |= static_cast<uint8>(PF::Bottom);
    if (str == "All")                          mask  = 0xFF;
    return mask;
}