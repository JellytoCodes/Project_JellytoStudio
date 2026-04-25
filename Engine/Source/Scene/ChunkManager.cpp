#include "Framework.h"
#include "ChunkManager.h"
#include "Entity/Components/Collider/AABBCollider.h"
#include "Entity/Components/Transform.h"

uint64 ChunkManager::CoordKey(int32 cx, int32 cz)
{
    return (static_cast<uint64>(static_cast<uint32>(cx)) << 32)
         |  static_cast<uint64>(static_cast<uint32>(cz));
}

void ChunkManager::WorldToChunkCoord(const Vec3& pos, int32& outCX, int32& outCZ)
{
    outCX = static_cast<int32>(std::floor(pos.x / kChunkSize));
    outCZ = static_cast<int32>(std::floor(pos.z / kChunkSize));
}

ChunkManager::Chunk& ChunkManager::GetOrCreateChunk(int32 cx, int32 cz)
{
    return _chunks[CoordKey(cx, cz)];
}

void ChunkManager::Chunk::RebuildAABB()
{
    if (entities.empty()) { aabbDirty = false; return; }

    Vec3 minP( FLT_MAX,  FLT_MAX,  FLT_MAX);
    Vec3 maxP(-FLT_MAX, -FLT_MAX, -FLT_MAX);

    for (Entity* e : entities)
    {
        auto* col = e->GetComponent<AABBCollider>();
        if (!col) continue;

        const DirectX::BoundingBox& bb = col->GetBoundingBox();
        const Vec3 c = { bb.Center.x,  bb.Center.y,  bb.Center.z  };
        const Vec3 x = { bb.Extents.x, bb.Extents.y, bb.Extents.z };

        minP.x = std::min(minP.x, c.x - x.x);
        minP.y = std::min(minP.y, c.y - x.y);
        minP.z = std::min(minP.z, c.z - x.z);
        maxP.x = max(maxP.x, c.x + x.x);
        maxP.y = max(maxP.y, c.y + x.y);
        maxP.z = max(maxP.z, c.z + x.z);
    }

    aabb.Center  = { (minP.x + maxP.x) * 0.5f,
                     (minP.y + maxP.y) * 0.5f,
                     (minP.z + maxP.z) * 0.5f };
    aabb.Extents = { (maxP.x - minP.x) * 0.5f,
                     (maxP.y - minP.y) * 0.5f,
                     (maxP.z - minP.z) * 0.5f };
    aabbDirty = false;
}

void ChunkManager::Register(Entity* entity)
{
    if (!entity) return;
    if (_entityToKey.count(entity)) return;

    auto* tr = entity->GetComponent<Transform>();
    if (!tr) return;

    int32 cx, cz;
    WorldToChunkCoord(tr->GetLocalPosition(), cx, cz);
    const uint64 key = CoordKey(cx, cz);

    Chunk& chunk = GetOrCreateChunk(cx, cz);
    chunk.entities.push_back(entity);
    chunk.aabbDirty = true;

    _entityToKey[entity] = key;
}

void ChunkManager::Unregister(Entity* entity)
{
    auto it = _entityToKey.find(entity);
    if (it == _entityToKey.end()) return;

    const uint64 key = it->second;
    _entityToKey.erase(it);

    auto chunkIt = _chunks.find(key);
    if (chunkIt == _chunks.end()) return;

    Chunk& chunk = chunkIt->second;
    chunk.entities.erase(std::remove(chunk.entities.begin(), chunk.entities.end(), entity), chunk.entities.end());
    chunk.aabbDirty = true;

    if (chunk.entities.empty())
        _chunks.erase(chunkIt);
}

void ChunkManager::Clear()
{
    _chunks.clear();
    _entityToKey.clear();
}

void ChunkManager::CollectVisible(const DirectX::BoundingFrustum& frustum, std::vector<Entity*>& outEntities)
{
    for (auto& [key, chunk] : _chunks)
    {
        if (chunk.entities.empty()) continue;

        if (chunk.aabbDirty)
            chunk.RebuildAABB();

        if (frustum.Contains(chunk.aabb) == DirectX::DISJOINT)
            continue;

        for (Entity* e : chunk.entities)
            outEntities.push_back(e);
    }
}

bool ChunkManager::IsManaged(Entity* entity) const
{
    return _entityToKey.count(entity) > 0;
}