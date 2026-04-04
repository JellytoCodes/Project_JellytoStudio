#pragma once

class IBlockPlacer
{
public:
    virtual ~IBlockPlacer() = default;

    virtual const std::vector<std::pair<int32, int32>>& GetPlacedBlocks() const = 0;

    virtual bool PlaceBlock(int32 col, int32 row) = 0;

    virtual void ClearAllBlocks() = 0;
};