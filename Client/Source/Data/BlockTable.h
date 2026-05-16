#pragma once
#include "Entity/Components/Collider/CollisionChannel.h"
#include "UI/PaletteWidget.h"

enum class BlockRenderType : uint8 { Mesh, Model };

enum class ColliderSize : uint8 { Small, Unit, Tall, Wide };

struct BlockUVRect
{
    float uOffset = 0.f;
    float vOffset = 0.f;
    float uScale  = 0.f;
    float vScale  = 0.f;
};
static_assert(sizeof(BlockUVRect) == 16,
    "HLSL float4 와 크기가 달라 셰이더 상수 버퍼 오정렬 발생");

struct BlockRecord
{
    int32            typeId        = 0;
    std::wstring     key;          
    std::wstring     label;        
    std::wstring     paletteLabel;

    Color            color;
    bool             isEraser      = false;

    BlockRenderType  renderType    = BlockRenderType::Mesh;
    std::wstring     modelName;   
    float            modelScale    = 0.01f;
    BlockUVRect      paletteRect; 

    ColliderSize     collider      = ColliderSize::Unit;
    CollisionChannel ownChannel    = CollisionChannel::Default;
    uint8            pickableMask  = 0xFF;
    uint8            faceMask      = 0xFF;
};

struct PhaseRecord
{
    PaletteWidget::SlotType dropSlot;
    std::wstring            modelName;
    std::wstring            phaseName;
    int32                   breaksToNext = 0;
};

class BlockTable
{
    DECLARE_SINGLE(BlockTable);

public:
    void Load(const std::wstring& jsonPath);

    bool IsLoaded() const { return _loaded; }

    const BlockRecord*              GetRecord(int32 typeId)                const;
    const BlockRecord*              GetRecord(PaletteWidget::SlotType st)  const;
    const BlockRecord*              GetRecordByKey(const std::wstring& key) const;

    const std::vector<BlockRecord>& GetAllRecords()    const { return _records;     }
    const std::vector<PhaseRecord>& GetPhaseSequence() const { return _phases;      }
    const std::wstring&             GetPalettePath()   const { return _palettePath; }
    const std::vector<BlockUVRect>& GetUVRects()       const { return _uvRects;     }

private:
    std::vector<BlockRecord>                              _records;
    std::unordered_map<std::wstring, const BlockRecord*>  _keyMap;
    std::vector<PhaseRecord>                              _phases;
    std::vector<BlockUVRect>                              _uvRects;
    std::wstring                                          _palettePath;
    bool                                                  _loaded = false;
};