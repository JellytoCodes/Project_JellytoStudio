#pragma once
#include "Component.h"

class Material;
class Mesh;
class AABBCollider;

// 타일 한 칸의 데이터
struct TileData
{
    bool walkable = true;  // 이동/배치 가능 여부
    int  type = 0;     // 타일 타입 (0=기본, 확장 가능)
};

// ── TileMap ────────────────────────────────────────────────────────────────
// - 그리드 렌더링 (MeshRenderer 자동 생성)
// - 타일 단위 walkable 데이터 관리
// - 월드 ↔ 그리드 좌표 변환 + 그리드 스냅
// - AABB Collider 자동 추가 (타일맵 전체 범위)
//   → PickGroundPoint 대신 TileMap Collider로 우클릭 피킹 가능
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

    // 그리드 생성 — MeshRenderer + AABB Collider 자동 추가
    void Create(int32 cols, int32 rows, float tileSize, std::shared_ptr<Material> material);

    // ── 타일 접근 (그리드 좌표) ──────────────────────────────────
    bool IsValid(int32 col, int32 row) const;

    TileData& GetTile(int32 col, int32 row);
    const TileData& GetTile(int32 col, int32 row) const;

    void SetWalkable(int32 col, int32 row, bool walkable);
    bool IsWalkable(int32 col, int32 row) const;

    // ── 월드 좌표 기반 조회 ──────────────────────────────────────
    // 월드 좌표가 타일맵 범위 안에 있는지
    bool IsInsideBounds(const Vec3& worldPos) const;

    // 월드 좌표 기준 walkable 여부 (범위 밖이면 false)
    bool IsWalkableWorld(const Vec3& worldPos) const;

    // 월드 좌표 → 타일 중심 월드 좌표로 스냅 (1칸 단위)
    // 범위 밖이면 false
    bool SnapToGrid(const Vec3& worldPos, Vec3& outSnapped) const;

    // ── 좌표 변환 ────────────────────────────────────────────────
    // 그리드 좌표 → 타일 중심 월드 좌표
    Vec3 GridToWorld(int32 col, int32 row) const;

    // 월드 좌표 → 그리드 좌표 (범위 밖이면 false)
    bool WorldToGrid(const Vec3& worldPos, int32& outCol, int32& outRow) const;

    // ── Getter ───────────────────────────────────────────────────
    int32 GetCols()     const { return _cols; }
    int32 GetRows()     const { return _rows; }
    float GetTileSize() const { return _tileSize; }

    // 타일맵 경계 (AABB 생성/범위 체크용)
    Vec3 GetBoundsMin() const;
    Vec3 GetBoundsMax() const;

private:
    int32 _cols = 0;
    int32 _rows = 0;
    float _tileSize = 1.f;

    std::vector<TileData> _tiles; // [row * _cols + col]
    std::shared_ptr<Mesh> _mesh;
};