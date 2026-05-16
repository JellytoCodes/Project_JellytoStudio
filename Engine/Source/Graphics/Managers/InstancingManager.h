#pragma once
#include "Entity/Entity.h"
#include "Pipeline/InstancingBuffer.h"
#include "Graphics/RenderPacket.h"

struct InstanceIDHash
{
    std::size_t operator()(const InstanceID& id) const noexcept
    {
        std::size_t h = std::hash<uint64>{}(id.resource0);
        h ^= std::hash<uint64>{}(id.resource1) + 0x9e3779b97f4a7c15ULL + (h << 6) + (h >> 2);
        h ^= std::hash<uint64>{}(id.bucket)    + 0x9e3779b97f4a7c15ULL + (h << 6) + (h >> 2);
        return h;
    }
};

struct RenderStats
{
    uint32 modelDrawCalls  = 0;
    uint32 meshDrawCalls   = 0;
    uint32 totalDrawCalls  = 0;
    uint32 totalInstances  = 0;

    uint32 dynamicBuffers  = 0;
    uint32 staticBuffers   = 0;

    uint32 meshGroupsRebuilt = 0;
    uint32 meshGroupsSkipped = 0;
};

class InstancingManager
{
    DECLARE_SINGLE(InstancingManager);

public:
    const RenderStats& GetStats() const { return _stats; }

    void Render   (std::vector<Entity*>& entities);
    void ClearData();

    void SetDirty        () { _bDirty        = true; }
    void SetMeshDirty    () { _meshDirty      = true; }
    void SetMeshGroupDirty() { _meshGroupDirty = true; }

    InstanceID GetMeshInstanceID(Entity* entity) const;

    void MarkMeshDirty(InstanceID id)
    {
        if (!id.IsValid()) return;
        _partialDirtyMesh.insert(id);
        _dynamicMeshIds.insert(id);
    }

    void MarkModelDirty(InstanceID id)
    {
        if (!id.IsValid()) return;
        _partialDirtyModel.insert(id);
    }

    // 청크 키를 포함한 정확한 InstanceID 로 dirty 마크.
    // TickPlaceTweens 등 엔티티 포인터를 보유한 호출처에서 사용한다.
    void MarkEntityMeshDirty(Entity* entity);

    void DumpInstancingStats() const;

private:
    void RenderMeshRenderer ();
    void RenderModelRenderer();
    void RenderAnimRenderer ();

    void PruneEmptyGroups();

    void AddData(InstanceID instanceID, const InstancingData& data, bool isDynamic = false);

    InstancingBuffer& GetOrCreateMeshBuffer(const InstanceID& id);

    void SmartRebuildMeshGroups(std::vector<Entity*>& entities);

    using BufferMap   = std::unordered_map<InstanceID, std::unique_ptr<InstancingBuffer>, InstanceIDHash>;
    using EntityCache = std::unordered_map<InstanceID, std::vector<Entity*>,              InstanceIDHash>;
    using WorldCache  = std::unordered_map<InstanceID, std::vector<InstancingData>,       InstanceIDHash>;
    using DirtySet    = std::unordered_set<InstanceID,                                    InstanceIDHash>;

    BufferMap   _buffers;

    EntityCache _meshCache;
    EntityCache _modelCache;
    EntityCache _animCache;

    WorldCache  _meshWorldCache;
    WorldCache  _modelWorldCache;

    bool _bDirty         = true;
    bool _meshDirty      = true;
    bool _meshGroupDirty = false;

    DirtySet _partialDirtyMesh;
    DirtySet _partialDirtyModel;

    DirtySet _dynamicMeshIds;

    RenderStats _stats;
};