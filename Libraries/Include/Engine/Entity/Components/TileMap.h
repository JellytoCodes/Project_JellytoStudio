#pragma once
#include "Component.h"

class Material;
class Mesh;
class AABBCollider;

struct TileData
{
    bool walkable = true;
    int  type = 0;
};

class TileMap : public Component
{
    using Super = Component;
public:
    TileMap();
    virtual ~TileMap() = default;

    virtual void Awake()      override {}
    virtual void Start()      override {}
    virtual void Update()     override {}
    virtual void LateUpdate() override {}
    virtual void OnDestroy()  override {}

    void Create(int32 cols, int32 rows, float tileSize, std::shared_ptr<Material> material);

    bool IsValid(int32 col, int32 row) const;

    TileData& GetTile(int32 col, int32 row);
    const TileData& GetTile(int32 col, int32 row) const;

    void SetWalkable(int32 col, int32 row, bool walkable);

    bool IsWalkable(int32 col, int32 row) const;
    bool IsInsideBounds(const Vec3& worldPos) const;
    bool IsWalkableWorld(const Vec3& worldPos) const;

    bool SnapToGrid(const Vec3& worldPos, Vec3& outSnapped) const;

    Vec3 GridToWorld(int32 col, int32 row) const;

    bool WorldToGrid(const Vec3& worldPos, int32& outCol, int32& outRow) const;

    int32 GetCols()     const { return _cols; }
    int32 GetRows()     const { return _rows; }
    float GetTileSize() const { return _tileSize; }

    Vec3 GetBoundsMin() const;
    Vec3 GetBoundsMax() const;

private:
    int32                   _cols = 0;
    int32                   _rows = 0;
    float                   _tileSize = 1.f;

    std::vector<TileData>   _tiles;
    std::shared_ptr<Mesh>   _mesh;
};