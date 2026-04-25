#pragma once
#include "Entity/Entity.h"

class ChunkManager
{
    DECLARE_SINGLE(ChunkManager);

public:
    static constexpr float kChunkSize = 16.0f;

    void Register  (Entity* entity);
    void Unregister(Entity* entity);
    void Clear();

    void CollectVisible(
        const DirectX::BoundingFrustum& frustum,
        std::vector<Entity*>&           outEntities);

    bool IsManaged(Entity* entity) const;

private:
    struct Chunk
    {
        std::vector<Entity*>  entities;
        DirectX::BoundingBox  aabb        = {};
        bool                  aabbDirty   = true;

        void RebuildAABB();
    };

    static uint64    CoordKey(int32 cx, int32 cz);
    static void      WorldToChunkCoord(const Vec3& pos, int32& outCX, int32& outCZ);

    Chunk& GetOrCreateChunk(int32 cx, int32 cz);

    std::unordered_map<uint64, Chunk>  _chunks;
    std::unordered_map<Entity*, uint64> _entityToKey;
};