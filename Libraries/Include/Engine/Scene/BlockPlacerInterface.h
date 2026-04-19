#pragma once

struct PlacedBlockRecord
{
    float x    = 0.f;
    float y    = 0.f;
    float z    = 0.f;
    int32 type = 0;
};

class IBlockPlacer
{
public:
    virtual ~IBlockPlacer() = default;

    virtual const std::vector<PlacedBlockRecord>& GetPlacedBlocks() const = 0;
    virtual bool PlaceBlock(float x, float y, float z, int32 type) = 0;
    virtual void ClearAllBlocks() = 0;
};