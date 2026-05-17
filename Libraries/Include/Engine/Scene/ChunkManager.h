#pragma once
#include "Entity/Entity.h"
#include "Entity/Components/Collider/CollisionChannel.h"

class BaseCollider;

struct ChunkSnapshot
{
    int32  cx          = 0;
    int32  cz          = 0;
    int32  entityCount = 0;
    bool   wasVisible  = false;
    DirectX::BoundingBox aabb = {};
};

class ChunkManager
{
    DECLARE_SINGLE(ChunkManager);

public:
    static constexpr float kChunkSize = 16.0f;

    void Register  (Entity* entity);
    void Unregister(Entity* entity);
    void MarkDirty (Entity* entity);
    void Clear();

    void CollectVisible(const DirectX::BoundingFrustum& frustum, std::vector<Entity*>& outEntities);
    void CollectStaticColliders(const DirectX::BoundingBox& bounds, std::vector<BaseCollider*>& outColliders);

    bool PickBlock (const Vec3& rayOrigin, const Vec3& rayDir, CollisionChannel queryChan, Entity*& outEntity, Vec3& outHitNormal, float& outDist);
    bool PickBlocks(const Vec3& rayOrigin, const Vec3& rayDir, uint8 queryMask, BlockPickHit& priming, BlockPickHit& floor, BlockPickHit& mushroom);

    bool IsManaged     (Entity* entity) const;
    bool TryGetChunkKey(Entity* entity, uint64& outKey) const;

    int32 GetChunkCount() const { return static_cast<int32>(_chunks.size()); }

    int32 GetVisibleChunkCount() const { return _lastVisibleCount; }

    std::vector<ChunkSnapshot> GetChunkSnapshots() const;

    bool TryGetChunkSnapshot(Entity* entity, ChunkSnapshot& outSnapshot) const;

private:
    struct Chunk
    {
        std::vector<Entity*>  entities;
        DirectX::BoundingBox  aabb        = {};
        bool                  aabbDirty   = true;
        bool                  wasVisible  = false;

        void RebuildAABB();
    };

    static void KeyToCoord(uint64 key, int32& outCX, int32& outCZ);

    static uint64 CoordKey(int32 cx, int32 cz);
    static void   WorldToChunkCoord(const Vec3& pos, int32& outCX, int32& outCZ);

    Chunk& GetOrCreateChunk(int32 cx, int32 cz);

    std::unordered_map<uint64, Chunk>   _chunks;
    std::unordered_map<Entity*, uint64> _entityToKey;

    int32 _lastVisibleCount = 0;
};