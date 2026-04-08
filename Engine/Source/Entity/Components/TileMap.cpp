#include "Framework.h"
#include "TileMap.h"

#include "MeshRenderer.h"
#include "Entity/Entity.h"
#include "Entity/Components/Transform.h"
#include "Entity/Components/Collider/AABBCollider.h"
#include "Graphics/Graphics.h"
#include "Resource/Mesh.h"

TileMap::TileMap()
    : Super(ComponentType::TileMap)
{
}

void TileMap::Create(int32 cols, int32 rows, float tileSize, std::shared_ptr<Material> material)
{
    _cols = cols;
    _rows = rows;
    _tileSize = tileSize;
    _tiles.assign(cols * rows, TileData{});

    auto entity = _entity.lock();
    if (!entity) return;

    // MeshRenderer 없으면 추가
    if (!entity->GetComponent<MeshRenderer>())
        entity->AddComponent(std::make_shared<MeshRenderer>());

    // 그리드 메시 생성 (cols x rows 단위 격자)
    _mesh = std::make_shared<Mesh>();
    _mesh->CreateGrid(Graphics::Get()->GetDevice(), cols, rows);

    auto mr = entity->GetComponent<MeshRenderer>();
    mr->SetMesh(_mesh);
    mr->SetPass(0);
    mr->SetMaterial(material);

    // 스케일로 타일 크기 반영
    entity->GetComponent<Transform>()->SetLocalScale(Vec3(tileSize, 1.f, tileSize));

    // ── AABB Collider 자동 추가 (타일맵 전체 범위) ──────────────
    // 기존 Collider가 없을 때만 추가
    // Extents = (cols*tileSize/2, 얇은두께, rows*tileSize/2)
    // Center offset은 그리드 메시가 (0,0)~(cols,rows) 범위라
    // 중심이 (cols/2, 0, rows/2)에 있으므로 offset으로 보정
    if (entity->GetComponent<AABBCollider>() == nullptr)
    {
        auto col = std::make_shared<AABBCollider>();
        float hw = cols * tileSize * 0.5f;
        float hd = rows * tileSize * 0.5f;
        col->SetBoxExtents(Vec3(hw, 0.05f, hd));               // 두께 0.05 (얇은 평면)
        col->SetOffsetPosition(Vec3(hw, 0.f, hd));             // 메시 중심으로 오프셋
        col->SetShowDebug(false);                               // 디버그 박스 숨김 (너무 큼)
        entity->AddComponent(col);
    }
}

// ── 유효성 ────────────────────────────────────────────────────────────────

bool TileMap::IsValid(int32 col, int32 row) const
{
    return col >= 0 && col < _cols && row >= 0 && row < _rows;
}

// ── 타일 접근 ─────────────────────────────────────────────────────────────

TileData& TileMap::GetTile(int32 col, int32 row)
{
    assert(IsValid(col, row));
    return _tiles[row * _cols + col];
}

const TileData& TileMap::GetTile(int32 col, int32 row) const
{
    assert(IsValid(col, row));
    return _tiles[row * _cols + col];
}

void TileMap::SetWalkable(int32 col, int32 row, bool walkable)
{
    if (IsValid(col, row))
        _tiles[row * _cols + col].walkable = walkable;
}

bool TileMap::IsWalkable(int32 col, int32 row) const
{
    if (!IsValid(col, row)) return false;
    return _tiles[row * _cols + col].walkable;
}

// ── 경계 ──────────────────────────────────────────────────────────────────

Vec3 TileMap::GetBoundsMin() const
{
    auto entity = _entity.lock();
    Vec3 origin = entity ? entity->GetTransform()->GetPosition() : Vec3::Zero;
    return Vec3(origin.x, origin.y, origin.z);
}

Vec3 TileMap::GetBoundsMax() const
{
    auto entity = _entity.lock();
    Vec3 origin = entity ? entity->GetTransform()->GetPosition() : Vec3::Zero;
    return Vec3(
        origin.x + _cols * _tileSize,
        origin.y,
        origin.z + _rows * _tileSize
    );
}

// ── 월드 좌표 기반 조회 ───────────────────────────────────────────────────

bool TileMap::IsInsideBounds(const Vec3& worldPos) const
{
    Vec3 mn = GetBoundsMin();
    Vec3 mx = GetBoundsMax();
    return worldPos.x >= mn.x && worldPos.x <= mx.x
        && worldPos.z >= mn.z && worldPos.z <= mx.z;
}

bool TileMap::IsWalkableWorld(const Vec3& worldPos) const
{
    int32 col, row;
    if (!WorldToGrid(worldPos, col, row)) return false;
    return IsWalkable(col, row);
}

bool TileMap::SnapToGrid(const Vec3& worldPos, Vec3& outSnapped) const
{
    int32 col, row;
    if (!WorldToGrid(worldPos, col, row)) return false;
    outSnapped = GridToWorld(col, row);
    outSnapped.y = worldPos.y; // Y는 원본 유지
    return true;
}

// ── 좌표 변환 ─────────────────────────────────────────────────────────────

Vec3 TileMap::GridToWorld(int32 col, int32 row) const
{
    auto entity = _entity.lock();
    Vec3 origin = entity ? entity->GetTransform()->GetPosition() : Vec3::Zero;

    return Vec3(
        origin.x + (col + 0.5f) * _tileSize,
        origin.y,
        origin.z + (row + 0.5f) * _tileSize
    );
}

bool TileMap::WorldToGrid(const Vec3& worldPos, int32& outCol, int32& outRow) const
{
    auto entity = _entity.lock();
    Vec3 origin = entity ? entity->GetTransform()->GetPosition() : Vec3::Zero;

    float localX = worldPos.x - origin.x;
    float localZ = worldPos.z - origin.z;

    // 음수 좌표도 올바르게 처리 (floor)
    outCol = (int32)floorf(localX / _tileSize);
    outRow = (int32)floorf(localZ / _tileSize);

    return IsValid(outCol, outRow);
}