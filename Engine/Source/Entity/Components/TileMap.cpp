
#include "Framework.h"
#include "TileMap.h"

#include "MeshRenderer.h"
#include "Entity/Entity.h"
#include "Entity/Components/Transform.h"
#include "Graphics/Graphics.h"
#include "Resource/Mesh.h"

TileMap::TileMap()
    : Super(ComponentType::TileMap) // Terrain 슬롯 재사용 (ComponentType 확장 전)
{

}

void TileMap::Create(int32 cols, int32 rows, float tileSize, std::shared_ptr<Material> material)
{
    _cols     = cols;
    _rows     = rows;
    _tileSize = tileSize;
    _tiles.assign(cols * rows, TileData{});

    auto entity = _entity.lock();
    if (!entity) return;

    // Transform 없으면 추가
    if (!entity->GetComponent<Transform>())
        entity->AddComponent(std::make_shared<Transform>());

    // MeshRenderer 없으면 추가
    if (!entity->GetComponent<MeshRenderer>())
        entity->AddComponent(std::make_shared<MeshRenderer>());

    // 그리드 전체를 하나의 평면 메시로 생성
    // CreateGrid는 (sizeX × sizeZ) 크기의 단위 격자 메시
    _mesh = std::make_shared<Mesh>();
    _mesh->CreateGrid(Graphics::Get()->GetDevice(), cols, rows);

    auto mr = entity->GetComponent<MeshRenderer>();
    mr->SetMesh(_mesh);
    mr->SetPass(0);
    mr->SetMaterial(material);

    // 스케일로 타일 크기 반영
    entity->GetComponent<Transform>()->SetLocalScale(Vec3(tileSize, 1.f, tileSize));
}

// ── 유효성 검사 ──────────────────────────────────────────────────────────

bool TileMap::IsValid(int32 col, int32 row) const
{
    return col >= 0 && col < _cols && row >= 0 && row < _rows;
}

// ── 타일 접근 ────────────────────────────────────────────────────────────

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

// ── 좌표 변환 ────────────────────────────────────────────────────────────

Vec3 TileMap::GridToWorld(int32 col, int32 row) const
{
    auto entity = _entity.lock();
    Vec3 origin = entity ? entity->GetTransform()->GetPosition() : Vec3::Zero;

    // 타일 중심 = origin + (col + 0.5) * tileSize, y=origin.y, (row + 0.5) * tileSize
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

    outCol = (int32)(localX / _tileSize);
    outRow = (int32)(localZ / _tileSize);

    return IsValid(outCol, outRow);
}