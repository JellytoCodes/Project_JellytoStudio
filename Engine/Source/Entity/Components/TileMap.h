#pragma once
#include "Component.h"

class Material;
class Mesh;

// 타일 한 칸의 데이터
struct TileData
{
    bool  walkable = true;   // 이동 가능 여부
    int   type     = 0;      // 타일 타입 (0=기본, 확장 가능)
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

    // 그리드 생성 (기존 데이터 초기화됨)
    void Create(int32 cols, int32 rows, float tileSize, std::shared_ptr<Material> material);

    // ── 타일 접근 ────────────────────────────────────────────────────
    bool IsValid(int32 col, int32 row) const;

    TileData&       GetTile(int32 col, int32 row);
    const TileData& GetTile(int32 col, int32 row) const;

    void SetWalkable(int32 col, int32 row, bool walkable);
    bool IsWalkable(int32 col, int32 row) const;

    // ── 좌표 변환 ────────────────────────────────────────────────────
    // 그리드 좌표 → 월드 좌표 (타일 중심)
    Vec3  GridToWorld(int32 col, int32 row) const;

    // 월드 좌표 → 그리드 좌표 (범위 밖이면 false)
    bool  WorldToGrid(const Vec3& worldPos, int32& outCol, int32& outRow) const;

    // ── Getter ───────────────────────────────────────────────────────
    int32 GetCols()     const { return _cols;     }
    int32 GetRows()     const { return _rows;     }
    float GetTileSize() const { return _tileSize; }

private:
    int32 _cols     = 0;
    int32 _rows     = 0;
    float _tileSize = 1.f;

    std::vector<TileData> _tiles; // [row * _cols + col]

    std::shared_ptr<Mesh> _mesh;
};