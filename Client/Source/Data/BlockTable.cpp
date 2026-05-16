#include "pch.h"
#include "BlockTable.h"

// nlohmann/json — Framework.h(→pch.h) 를 통해 포함됨
using json    = nlohmann::json;
using SlotType = PaletteWidget::SlotType;
using CH       = CollisionChannel;
using PF       = PlaceFace;

// ─────────────────────────────────────────────────────────────────────────────
// 내부 파싱 헬퍼 — 헤더에 노출하지 않음
// ─────────────────────────────────────────────────────────────────────────────
namespace
{
    // UTF-8 std::string → wstring
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

    // wstring → UTF-8 std::string (파일 경로 전달용)
    std::string WideToUtf8(const std::wstring& w)
    {
        if (w.empty()) return {};
        const int n = ::WideCharToMultiByte(
            CP_UTF8, 0, w.c_str(), -1, nullptr, 0, nullptr, nullptr);
        std::string s(static_cast<size_t>(n - 1), '\0');
        ::WideCharToMultiByte(CP_UTF8, 0, w.c_str(), -1, s.data(), n, nullptr, nullptr);
        return s;
    }

    // ── 문자열 열거형 변환 ────────────────────────────────────────────────────

    ColliderSize ParseCollider(const std::string& s)
    {
        if (s == "Small") return ColliderSize::Small;
        if (s == "Tall")  return ColliderSize::Tall;
        if (s == "Wide")  return ColliderSize::Wide;
        return ColliderSize::Unit;
    }

    CollisionChannel ParseChannel(const std::string& s)
    {
        if (s == "Priming")   return CH::Priming;
        if (s == "Floor")     return CH::Floor;
        if (s == "Mushroom")  return CH::Mushroom;
        if (s == "Character") return CH::Character;
        return CH::Default;
    }

    BlockRenderType ParseRenderType(const std::string& s)
    {
        return (s == "Model") ? BlockRenderType::Model : BlockRenderType::Mesh;
    }

    // ── JSON 배열 → 비트마스크 (pickable / faces) ────────────────────────────
    //
    // XML 시절: ParsePickable("Priming|Floor|Character") — 수동 find() 루프
    // JSON 이후: ParsePickable(["Priming","Floor","Character"]) — 배열 순회
    //
    // 파이프 파싱 코드 완전 제거, 가독성 및 유지보수성 향상.

    uint8 ParsePickable(const json& arr)
    {
        if (!arr.is_array() || arr.empty()) return 0xFF;
        if (arr.size() == 1 && arr[0].get<std::string>() == "All") return 0xFF;

        uint8 mask = 0;
        for (const auto& item : arr)
        {
            const std::string ch = item.get<std::string>();
            if      (ch == "Priming")   mask |= static_cast<uint8>(CH::Priming);
            else if (ch == "Floor")     mask |= static_cast<uint8>(CH::Floor);
            else if (ch == "Mushroom")  mask |= static_cast<uint8>(CH::Mushroom);
            else if (ch == "Character") mask |= static_cast<uint8>(CH::Character);
        }
        return mask ? mask : 0xFF;
    }

    uint8 ParseFaces(const json& arr)
    {
        if (!arr.is_array() || arr.empty()) return 0xFF;
        if (arr.size() == 1 && arr[0].get<std::string>() == "All") return 0xFF;

        uint8 mask = 0;
        for (const auto& item : arr)
        {
            const std::string face = item.get<std::string>();
            if      (face == "Top")  mask |= static_cast<uint8>(PF::Top);
            else if (face == "Side") mask |= static_cast<uint8>(PF::Side);
            else if (face == "Bot")  mask |= static_cast<uint8>(PF::Bottom);
        }
        return mask ? mask : 0xFF;
    }

    // ── JSON color 배열([R,G,B,A]) → Color ───────────────────────────────────
    Color ParseColor(const json& j, float defAlpha = 1.f)
    {
        if (!j.is_array() || j.size() < 3) return Color(0, 0, 0, defAlpha);
        return Color(
            j[0].get<float>(),
            j[1].get<float>(),
            j[2].get<float>(),
            j.size() >= 4 ? j[3].get<float>() : defAlpha);
    }
}

void BlockTable::Load(const std::wstring& jsonPath)
{
    assert(!_loaded && "BlockTable::Load() 는 앱 시작 시 1회만 호출해야 합니다.");

    std::ifstream file(WideToUtf8(jsonPath));
    assert(file.is_open() && "BlockMaster.json 을 찾을 수 없습니다.");
    if (!file.is_open()) return;

    json doc;
    try
    {
        file >> doc;
    }
    catch (const json::parse_error& e)
    {
        ::OutputDebugStringA(e.what());
        assert(false && "BlockMaster.json 파싱 실패 — JSON 문법 오류");
        return;
    }

    _palettePath = doc.contains("paletteTex")
        ? Utf8ToWide(doc["paletteTex"].get<std::string>())
        : L"../Resources/Textures/MapModel/Main_texture.png";

    assert(doc.contains("blocks") && "BlockMaster.json 에 \"blocks\" 키가 없습니다.");
    const auto& blocks = doc["blocks"];

    for (const auto& b : blocks)
    {
        if (!b.contains("id")) continue;

        BlockRecord rec;

        rec.typeId       = b["id"].get<int32>();
        rec.key          = Utf8ToWide(b.value("key",          ""));
        rec.label        = Utf8ToWide(b.value("label",        ""));
        rec.paletteLabel = Utf8ToWide(b.value("paletteLabel", ""));
        rec.isEraser     = b.value("isEraser", false);

        if (b.contains("color"))
            rec.color = ParseColor(b["color"]);

        rec.renderType = ParseRenderType(b.value("renderType", "Mesh"));

        if (rec.renderType == BlockRenderType::Model)
        {
            rec.modelName   = Utf8ToWide(b.value("modelName", ""));
            rec.modelScale  = b.value("modelScale", 0.01f);
            rec.paletteRect = { 0.f, 0.f, 1.f, 1.f };
        }
        else
        {
            rec.paletteRect.uOffset = b.value("paletteU", 0.f);
            rec.paletteRect.vOffset = b.value("paletteV", 0.f);
            rec.paletteRect.uScale  = 0.f;
            rec.paletteRect.vScale  = 0.f;
        }

        rec.collider    = ParseCollider(b.value("collider",   "Unit"));
        rec.ownChannel  = ParseChannel (b.value("ownChannel", "Default"));

        if (b.contains("pickable"))
            rec.pickableMask = ParsePickable(b["pickable"]);
        if (b.contains("faces"))
            rec.faceMask     = ParseFaces   (b["faces"]);

        const int32 slot = rec.typeId;
        if (slot >= static_cast<int32>(_records.size()))
        {
            _records.resize(static_cast<size_t>(slot + 1));
            _uvRects.resize(static_cast<size_t>(slot + 1), BlockUVRect{});
        }
        _records[slot] = rec;
        _uvRects [slot] = rec.paletteRect;
    }

    _keyMap.reserve(_records.size());
    for (const auto& rec : _records)
        if (!rec.key.empty()) _keyMap.emplace(rec.key, &rec);

    assert(doc.contains("phaseSequence") &&
           "BlockMaster.json 에 \"phaseSequence\" 키가 없습니다.");

    for (const auto& p : doc["phaseSequence"])
    {
        const std::wstring slotKey = Utf8ToWide(p.value("slotKey", ""));
        const BlockRecord* slotRec = GetRecordByKey(slotKey);
        assert(slotRec && "phaseSequence.slotKey 가 blocks 에 없습니다.");
        if (!slotRec) continue;

        PhaseRecord phase;
        phase.dropSlot     = static_cast<SlotType>(slotRec->typeId);
        phase.modelName    = slotRec->modelName;
        phase.phaseName    = Utf8ToWide(p.value("phaseName",   ""));
        phase.breaksToNext = p.value("breaksToNext", 0);
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