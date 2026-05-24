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

    Entity* entity = _entity;
    if (!entity) return;

    if (!entity->GetComponent<MeshRenderer>())
        entity->AddComponent(std::make_unique<MeshRenderer>());

    _mesh = std::make_shared<Mesh>();
    _mesh->CreateGrid(GET_SINGLE(Graphics)->GetDevice(), cols, rows);

    MeshRenderer* mr = entity->GetComponent<MeshRenderer>();
    mr->SetMesh(_mesh);
    mr->SetPass(0);
    mr->SetMaterial(material);

    entity->GetComponent<Transform>()->SetLocalScale(Vec3(tileSize, 1.f, tileSize));

	entity->AddComponent(std::make_unique<AABBCollider>());
	AABBCollider* col = entity->GetComponent<AABBCollider>();
	float hw = cols * tileSize * 0.5f;
	float hd = rows * tileSize * 0.5f;
	col->SetBoxExtents(Vec3(hw, 0.05f, hd));
	col->SetOffsetPosition(Vec3(hw, 0.f, hd));
	col->SetShowDebug(false);
}

bool TileMap::IsValid(int32 col, int32 row) const
{
    return col >= 0 && col < _cols && row >= 0 && row < _rows;
}

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

Vec3 TileMap::GetBoundsMin() const
{
    auto entity = _entity;
    Vec3 origin = entity ? entity->GetComponent<Transform>()->GetPosition() : Vec3::Zero;
    return Vec3(origin.x, origin.y, origin.z);
}

Vec3 TileMap::GetBoundsMax() const
{
    auto entity = _entity;
    Vec3 origin = entity ? entity->GetComponent<Transform>()->GetPosition() : Vec3::Zero;
    return Vec3(
        origin.x + _cols * _tileSize,
        origin.y,
        origin.z + _rows * _tileSize
    );
}

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
    outSnapped.y = worldPos.y;
    return true;
}

Vec3 TileMap::GridToWorld(int32 col, int32 row) const
{
    auto entity = _entity;
    Vec3 origin = entity ? entity->GetComponent<Transform>()->GetPosition() : Vec3::Zero;

    return Vec3(
        origin.x + (col + 0.5f) * _tileSize,
        origin.y,
        origin.z + (row + 0.5f) * _tileSize
    );
}

bool TileMap::WorldToGrid(const Vec3& worldPos, int32& outCol, int32& outRow) const
{
    auto entity = _entity;
    Vec3 origin = entity ? entity->GetComponent<Transform>()->GetPosition() : Vec3::Zero;

    float localX = worldPos.x - origin.x;
    float localZ = worldPos.z - origin.z;

    outCol = (int32)floorf(localX / _tileSize);
    outRow = (int32)floorf(localZ / _tileSize);

    return IsValid(outCol, outRow);
}