#pragma once
#include "Entity/Components/Collider/CollisionChannel.h"
#include "UI/PaletteWidget.h"

namespace tinyxml2 { class XMLElement; }

// ─────────────────────────────────────────────────────────────────────────────
// 렌더링 분기 타입
// ─────────────────────────────────────────────────────────────────────────────
enum class BlockRenderType : uint8 { Mesh, Model };

// ─────────────────────────────────────────────────────────────────────────────
// 충돌체 크기 프리셋
// ─────────────────────────────────────────────────────────────────────────────
enum class ColliderSize : uint8 { Small, Unit, Tall, Wide };

// ─────────────────────────────────────────────────────────────────────────────
// BlockUVRect  — 팔레트 텍스처 UV 좌표 (HLSL float4 g_AtlasRects 와 1:1)
//   Mesh 타입: uScale=0, vScale=0 → 팔레트 단일 픽셀 단색 샘플링
//   Model 타입: uScale=1, vScale=1 → 베이크된 UV 패스스루
// ─────────────────────────────────────────────────────────────────────────────
struct BlockUVRect
{
    float uOffset = 0.f;
    float vOffset = 0.f;
    float uScale  = 0.f;
    float vScale  = 0.f;
};
static_assert(sizeof(BlockUVRect) == 16, "HLSL float4 와 크기가 달라 셰이더 상수 버퍼 오정렬 발생");

// ─────────────────────────────────────────────────────────────────────────────
// BlockRecord  — BlockData.xml + BlockDefs.xml 의 통합 레코드
//   UI 정보 (label, color), 렌더링 정보 (renderType, modelName, paletteRect),
//   충돌 정보 (collider, ownChannel, pickableMask, faceMask) 를 하나로 통합
// ─────────────────────────────────────────────────────────────────────────────
struct BlockRecord
{
    // ── 식별 ────────────────────────────────────────────────────────────────
    int32            typeId        = 0;
    std::wstring     key;           // "Mushroom1", "Priming1", ...
    std::wstring     label;         // "버섯1", "블록1", ...
    std::wstring     paletteLabel;  // 팔레트 UI 표시 문자열

    // ── UI ──────────────────────────────────────────────────────────────────
    Color            color;
    bool             isEraser      = false;

    // ── 렌더링 ──────────────────────────────────────────────────────────────
    BlockRenderType  renderType    = BlockRenderType::Mesh;
    std::wstring     modelName;     // Model 타입: ReadModel/ReadMaterial 에 전달할 이름
    float            modelScale    = 0.01f;
    BlockUVRect      paletteRect;   // Mesh 타입: 팔레트 UV 좌표

    // ── 충돌 ────────────────────────────────────────────────────────────────
    ColliderSize     collider      = ColliderSize::Unit;
    CollisionChannel ownChannel    = CollisionChannel::Default;
    uint8            pickableMask  = 0xFF;
    uint8            faceMask      = 0xFF;
};

// ─────────────────────────────────────────────────────────────────────────────
// PhaseRecord  — 원블록 채굴 페이즈 시퀀스 한 항목
// ─────────────────────────────────────────────────────────────────────────────
struct PhaseRecord
{
    PaletteWidget::SlotType dropSlot;
    std::wstring            modelName;
    std::wstring            phaseName;
    int32                   breaksToNext = 0;
};

// ─────────────────────────────────────────────────────────────────────────────
// BlockTable  — 통합 블록 데이터 싱글톤
//   BlockMaster.xml 단일 파일에서 UI·렌더링·충돌 데이터를 모두 로드한다.
//   기존 BlockDataTable + BlockDefRegistry 를 완전 대체.
// ─────────────────────────────────────────────────────────────────────────────
class BlockTable
{
    DECLARE_SINGLE(BlockTable);

public:
    void Load(const std::wstring& xmlPath);

    bool IsLoaded() const { return _loaded; }

    const BlockRecord*              GetRecord(int32 typeId)               const;
    const BlockRecord*              GetRecord(PaletteWidget::SlotType st)  const;
    const BlockRecord*              GetRecordByKey(const std::wstring& key) const;

    const std::vector<BlockRecord>& GetAllRecords()      const { return _records; }
    const std::vector<PhaseRecord>& GetPhaseSequence()   const { return _phases;  }
    const std::wstring&             GetPalettePath()     const { return _palettePath; }
    const std::vector<BlockUVRect>& GetUVRects()         const { return _uvRects;  }

private:
    static std::wstring     Utf8ToWide (const char* utf8);
    static std::string      WideToUtf8 (const std::wstring& w);
    static float            AttrFloat  (const tinyxml2::XMLElement* e,
                                        const char* name, float def = 0.f);

    static ColliderSize      ParseCollider (const char* s);
    static CollisionChannel  ParseChannel  (const char* s);
    static uint8             ParsePickable (const char* s);
    static uint8             ParseFaces    (const char* s);
    static BlockRenderType   ParseRenderType(const char* s);

    std::vector<BlockRecord>                              _records;
    std::unordered_map<std::wstring, const BlockRecord*>  _keyMap;
    std::vector<PhaseRecord>                              _phases;
    std::vector<BlockUVRect>                              _uvRects;
    std::wstring                                          _palettePath;
    bool                                                  _loaded = false;
};