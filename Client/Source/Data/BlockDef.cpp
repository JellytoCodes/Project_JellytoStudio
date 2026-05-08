#include "pch.h"
#include "BlockDef.h"
#include "Utils/tinyxml2.h"

using CH = CollisionChannel;

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

bool BlockDefRegistry::Load(const std::wstring& xmlPath)
{
    tinyxml2::XMLDocument doc;
    if (doc.LoadFile(WStrToStr(xmlPath).c_str()) != tinyxml2::XML_SUCCESS)
    {
        ::OutputDebugStringW((L"[BlockDefRegistry] 파일 없음: " + xmlPath + L"\n").c_str());
        return false;
    }

    tinyxml2::XMLElement* root = doc.FirstChildElement("BlockDefs");
    if (!root) return false;

    if (const char* pal = root->Attribute("paletteTex"))
        _palettePath = StrToWStr(pal);

    _defs.clear();

    for (auto* elem = root->FirstChildElement("Block"); elem; elem = elem->NextSiblingElement("Block"))
    {
        BlockDef def;

        elem->QueryIntAttribute("id", &def.typeId);

        if (const char* n = elem->Attribute("name"))    def.name = StrToWStr(n);
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
            if (const char* mp = elem->Attribute("model"))
                def.modelPath = StrToWStr(mp);
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
    if (str.find("Top")  != std::string::npos) mask |= 0x01;
    if (str.find("Side") != std::string::npos) mask |= 0x02;
    if (str.find("Bot")  != std::string::npos) mask |= 0x04;
    if (str == "All")                          mask  = 0xFF;
    return mask;
}