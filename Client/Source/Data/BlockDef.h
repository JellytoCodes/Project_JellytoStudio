#pragma once
#include "Entity/Components/Collider/CollisionChannel.h"

enum class BlockRenderType : uint8
{
    Mesh,
    Model,
};

enum class ColliderSize : uint8
{
    Small,
    Unit,
    Tall,
    Wide,
};

struct BlockUVRect
{
    float uOffset = 0.f;
    float vOffset = 0.f;
    float uScale  = 0.f;
    float vScale  = 0.f;
};
static_assert(sizeof(BlockUVRect) == 16, "BlockUVRect must match float4 in HLSL");

struct BlockDef
{
    int32            typeId       = 0;
    std::wstring     name;
    BlockRenderType  renderType   = BlockRenderType::Mesh;

    BlockUVRect      paletteRect;

    std::wstring     modelPath;
    float            modelScale   = 1.f;

    ColliderSize     collider     = ColliderSize::Unit;
    CollisionChannel ownChannel   = CollisionChannel::Default;
    uint8            pickableMask = 0xFF;
    uint8            faceMask     = 0xFF;
};

class BlockDefRegistry
{
    DECLARE_SINGLE(BlockDefRegistry);

public:
    bool Load(const std::wstring& xmlPath);

    const BlockDef*                  GetDef(int32 typeId)  const;
    int32                            GetCount()             const { return static_cast<int32>(_defs.size()); }
    const std::wstring&              GetPalettePath()       const { return _palettePath; }
    const std::vector<BlockUVRect>&  GetUVRects()           const { return _uvRects; }

private:
    static ColliderSize      ParseCollider (const char* s);
    static CollisionChannel  ParseChannel  (const char* s);
    static uint8             ParsePickable (const char* s);
    static uint8             ParseFaces    (const char* s);

    std::vector<BlockDef>   _defs;
    std::vector<BlockUVRect> _uvRects;
    std::wstring             _palettePath;
};