#pragma once
#include "Entity/Entity.h"
#include "Pipeline/InstancingBuffer.h"
#include "Graphics/RenderPacket.h"

struct InstanceIDHash
{
    std::size_t operator()(const InstanceID& id) const noexcept
    {
        std::size_t h = std::hash<uint64>{}(id.first);
        h ^= std::hash<uint64>{}(id.second)
            + 0x9e3779b97f4a7c15ULL
            + (h << 6) + (h >> 2);
        return h;
    }
};

struct RenderStats
{
    uint32 modelDrawCalls  = 0;
    uint32 meshDrawCalls   = 0;
    uint32 totalDrawCalls  = 0;
    uint32 totalInstances  = 0;
};

class InstancingManager
{
    DECLARE_SINGLE(InstancingManager);

public:

	const RenderStats& GetStats() const { return _stats; }

    void Render(std::vector<Entity*>& entities);
    void ClearData();

    void SetDirty() { _bDirty = true; }
    void SetMeshDirty() { _meshDirty = true; }  // ★ Task 2와 연동

    void DumpInstancingStats() const;

private:
    void RenderMeshRenderer();
    void RenderModelRenderer();
    void RenderAnimRenderer();

    void AddData(InstanceID instanceID, const InstancingData& data);

    using BufferMap = std::unordered_map<InstanceID, std::unique_ptr<InstancingBuffer>, InstanceIDHash>;
    using EntityCache = std::unordered_map<InstanceID, std::vector<Entity*>, InstanceIDHash>;
    using WorldCache = std::unordered_map<InstanceID, std::vector<InstancingData>, InstanceIDHash>;

    BufferMap   _buffers;
    EntityCache _meshCache;
    EntityCache _modelCache;
    EntityCache _animCache;
    WorldCache  _modelWorldCache;
    WorldCache  _meshWorldCache;

    bool _bDirty = true;
    bool _meshDirty = true;

	RenderStats _stats;
};