#include "Framework.h"
#include "ChunkManager.h"
#include "Scene/PickUtils.h"

#include "Entity/Components/Collider/AABBCollider.h"
#include "Entity/Components/Transform.h"
#include "Entity/Components/MeshRenderer.h"
#include "Graphics/Model/ModelRenderer.h"

uint64 ChunkManager::CoordKey(int32 cx, int32 cz)
{
    return (static_cast<uint64>(static_cast<uint32>(cx)) << 32)
         |  static_cast<uint64>(static_cast<uint32>(cz));
}

void ChunkManager::KeyToCoord(uint64 key, int32& outCX, int32& outCZ)
{
    outCX = static_cast<int32>(static_cast<uint32>(key >> 32));
    outCZ = static_cast<int32>(static_cast<uint32>(key & 0xFFFFFFFFull));
}

void ChunkManager::WorldToChunkCoord(const Vec3& pos, int32& outCX, int32& outCZ)
{
    outCX = static_cast<int32>(std::floor(pos.x / kChunkSize));
    outCZ = static_cast<int32>(std::floor(pos.z / kChunkSize));
}

uint64 ChunkManager::PositionKey(const Vec3& pos)
{
    constexpr int32 kBias = 524288;
    const int32 ix = static_cast<int32>(std::round(pos.x)) + kBias;
    const int32 iy = static_cast<int32>(std::round(pos.y)) + kBias;
    const int32 iz = static_cast<int32>(std::round(pos.z)) + kBias;
    return (static_cast<uint64>(ix) << 40)
         | (static_cast<uint64>(iy) << 20)
         |  static_cast<uint64>(iz);
}

bool ChunkManager::HasSolidBlockAt(const Vec3& pos) const
{
    return _positionMap.count(PositionKey(pos)) > 0;
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
        maxP.x = std::max(maxP.x, c.x + x.x);
        maxP.y = std::max(maxP.y, c.y + x.y);
        maxP.z = std::max(maxP.z, c.z + x.z);
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

    const Vec3 pos = tr->GetLocalPosition();
    int32 cx, cz;
    WorldToChunkCoord(pos, cx, cz);
    const uint64 key = CoordKey(cx, cz);

    Chunk& chunk = GetOrCreateChunk(cx, cz);
    chunk.entities.push_back(entity);
    chunk.aabbDirty = true;

    _entityToKey[entity] = key;

    if (entity->GetComponent<MeshRenderer>() && !entity->GetComponent<ModelRenderer>())
        _positionMap[PositionKey(pos)] = entity;
}

void ChunkManager::Unregister(Entity* entity)
{
    auto it = _entityToKey.find(entity);
    if (it == _entityToKey.end()) return;

    const uint64 key = it->second;
    _entityToKey.erase(it);

    if (entity->GetComponent<MeshRenderer>() && !entity->GetComponent<ModelRenderer>())
    {
        if (auto* tr = entity->GetComponent<Transform>())
            _positionMap.erase(PositionKey(tr->GetLocalPosition()));
    }

    auto chunkIt = _chunks.find(key);
    if (chunkIt == _chunks.end()) return;

    Chunk& chunk = chunkIt->second;
    chunk.entities.erase(
        std::remove(chunk.entities.begin(), chunk.entities.end(), entity),
        chunk.entities.end());
    chunk.aabbDirty = true;

    if (chunk.entities.empty())
        _chunks.erase(chunkIt);
}

void ChunkManager::MarkDirty(Entity* entity)
{
    auto it = _entityToKey.find(entity);
    if (it == _entityToKey.end()) return;

    auto chunkIt = _chunks.find(it->second);
    if (chunkIt == _chunks.end()) return;

    chunkIt->second.aabbDirty = true;
}

void ChunkManager::Clear()
{
    _chunks.clear();
    _entityToKey.clear();
    _positionMap.clear();
    _lastVisibleCount = 0;
}

void ChunkManager::CollectVisible(const DirectX::BoundingFrustum& frustum, std::vector<Entity*>& outEntities)
{
    static constexpr Vec3 kNeighborOffsets[6] =
    {
        { 1.f, 0.f, 0.f }, {-1.f, 0.f, 0.f },
        { 0.f, 1.f, 0.f }, { 0.f,-1.f, 0.f },
        { 0.f, 0.f, 1.f }, { 0.f, 0.f,-1.f },
    };

    _lastVisibleCount = 0;

    for (auto& [key, chunk] : _chunks)
    {
        chunk.wasVisible = false;

        if (chunk.entities.empty()) continue;

        if (chunk.aabbDirty)
            chunk.RebuildAABB();

        if (frustum.Contains(chunk.aabb) == DirectX::DISJOINT)
            continue;

        chunk.wasVisible = true;
        ++_lastVisibleCount;

        for (Entity* e : chunk.entities)
        {
            auto* mr = e->GetComponent<MeshRenderer>();
            auto* tr = e->GetComponent<Transform>();

            if (!mr || !tr)
            {
                outEntities.push_back(e);
                continue;
            }

            const Vec3 pos = tr->GetLocalPosition();
            bool occluded = true;
            for (const Vec3& off : kNeighborOffsets)
            {
                if (!HasSolidBlockAt(pos + off))
                {
                    occluded = false;
                    break;
                }
            }

            if (!occluded)
                outEntities.push_back(e);
        }
    }
}

void ChunkManager::CollectStaticColliders(const DirectX::BoundingBox& bounds,
                                          std::vector<BaseCollider*>& outColliders)
{
    const Vec3 center  = { bounds.Center.x,  bounds.Center.y,  bounds.Center.z  };
    const Vec3 extents = { bounds.Extents.x, bounds.Extents.y, bounds.Extents.z };
    const Vec3 minP = center - extents;
    const Vec3 maxP = center + extents;

    int32 minCX, minCZ, maxCX, maxCZ;
    WorldToChunkCoord(minP, minCX, minCZ);
    WorldToChunkCoord(maxP, maxCX, maxCZ);

    for (int32 cz = minCZ; cz <= maxCZ; ++cz)
    {
        for (int32 cx = minCX; cx <= maxCX; ++cx)
        {
            auto it = _chunks.find(CoordKey(cx, cz));
            if (it == _chunks.end()) continue;

            Chunk& chunk = it->second;
            if (chunk.entities.empty()) continue;

            if (chunk.aabbDirty)
                chunk.RebuildAABB();

            if (!chunk.aabb.Intersects(bounds))
                continue;

            for (Entity* e : chunk.entities)
            {
                auto* col = e->GetComponent<AABBCollider>();
                if (!col || !col->IsStatic()) continue;
                outColliders.push_back(col);
            }
        }
    }
}

bool ChunkManager::IsManaged(Entity* entity) const
{
    return _entityToKey.count(entity) > 0;
}

bool ChunkManager::TryGetChunkKey(Entity* entity, uint64& outKey) const
{
    auto it = _entityToKey.find(entity);
    if (it == _entityToKey.end()) return false;
    outKey = it->second;
    return true;
}

std::vector<ChunkSnapshot> ChunkManager::GetChunkSnapshots() const
{
    std::vector<ChunkSnapshot> result;
    result.reserve(_chunks.size());

    for (const auto& [key, chunk] : _chunks)
    {
        ChunkSnapshot s;
        KeyToCoord(key, s.cx, s.cz);
        s.entityCount = static_cast<int32>(chunk.entities.size());
        s.wasVisible  = chunk.wasVisible;
        s.aabb        = chunk.aabb;
        result.push_back(s);
    }

    std::sort(result.begin(), result.end(), [](const ChunkSnapshot& a, const ChunkSnapshot& b)
    {
        if (a.cz != b.cz) return a.cz < b.cz;
        return a.cx < b.cx;
    });

    return result;
}

bool ChunkManager::TryGetChunkSnapshot(Entity* entity, ChunkSnapshot& outSnapshot) const
{
    auto it = _entityToKey.find(entity);
    if (it == _entityToKey.end()) return false;

    const uint64 key    = it->second;
    auto chunkIt = _chunks.find(key);
    if (chunkIt == _chunks.end()) return false;

    const Chunk& chunk = chunkIt->second;
    KeyToCoord(key, outSnapshot.cx, outSnapshot.cz);
    outSnapshot.entityCount = static_cast<int32>(chunk.entities.size());
    outSnapshot.wasVisible  = chunk.wasVisible;
    outSnapshot.aabb        = chunk.aabb;
    return true;
}

bool ChunkManager::PickBlock(const Vec3& rayOrigin, const Vec3& rayDir, CollisionChannel queryChan, Entity*& outEntity, Vec3& outHitNormal, float& outDist)
{
    outEntity    = nullptr;
    outDist      = FLT_MAX;
    outHitNormal = Vec3(0, 1, 0);

    const XMVECTOR vOrigin = XMLoadFloat3(&rayOrigin);
    const XMVECTOR vDir    = XMLoadFloat3(&rayDir);

    for (auto& [key, chunk] : _chunks)
    {
        if (chunk.entities.empty()) continue;
        if (chunk.aabbDirty) chunk.RebuildAABB();

        float chunkDist = 0.f;
        if (!chunk.aabb.Intersects(vOrigin, vDir, chunkDist)) continue;
        if (chunkDist >= outDist) continue;

        for (Entity* entity : chunk.entities)
        {
            auto* aabb = entity->GetComponent<AABBCollider>();
            if (!aabb) continue;
            if (!aabb->CanBePickedBy(queryChan)) continue;

            float dist = 0.f;
            Vec3  normal;
            Vec3  origin = rayOrigin, dir = rayDir;
            Ray   r(origin, dir);

            if (aabb->IntersectsWithNormal(r, dist, normal) && dist < outDist)
            {
                outDist      = dist;
                outEntity    = entity;
                outHitNormal = normal;
            }
        }
    }

    return outEntity != nullptr;
}

bool ChunkManager::PickBlocks(const Vec3& rayOrigin, const Vec3& rayDir, uint8 queryMask, BlockPickHit& priming, BlockPickHit& floor, BlockPickHit& mushroom)
{
    const XMVECTOR vOrigin = XMLoadFloat3(&rayOrigin);
    const XMVECTOR vDir    = XMLoadFloat3(&rayDir);

    float bestKnownDist = FLT_MAX;

    for (auto& [key, chunk] : _chunks)
    {
        if (chunk.entities.empty()) continue;
        if (chunk.aabbDirty) chunk.RebuildAABB();

        bestKnownDist = std::min({ priming.dist, floor.dist, mushroom.dist });

        float chunkDist = 0.f;
        if (!chunk.aabb.Intersects(vOrigin, vDir, chunkDist)) continue;
        if (chunkDist >= bestKnownDist) continue;

        for (Entity* entity : chunk.entities)
        {
            auto* aabb = entity->GetComponent<AABBCollider>();
            if (!aabb) continue;
            if ((aabb->GetPickableMask() & queryMask) == 0) continue;

            float dist = 0.f;
            Vec3  normal;
            Vec3  origin = rayOrigin, dir = rayDir;
            Ray   r(origin, dir);

            if (aabb->IntersectsWithNormal(r, dist, normal))
                UpdateMatchingPickHits(queryMask, aabb, entity, normal, dist,
                                       priming, floor, mushroom);
        }
    }

    return priming.valid || floor.valid || mushroom.valid;
}