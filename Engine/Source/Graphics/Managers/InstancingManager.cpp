#include "Framework.h"
#include "InstancingManager.h"

#include "Entity/Entity.h"
#include "Entity/Components/Transform.h"
#include "Entity/Components/MeshRenderer.h"
#include "Graphics/Model/ModelRenderer.h"
#include "Graphics/Model/ModelAnimator.h"
#include "Pipeline/Shader.h"
#include "Pipeline/DynamicInstancePool.h"
#include "Scene/ChunkManager.h"

InstanceID InstancingManager::GetMeshInstanceID(Entity* entity) const
{
    if (!entity) return {};

    auto* mr = entity->GetComponent<MeshRenderer>();
    if (!mr) return {};

    InstanceID id = mr->GetInstanceID();

    uint64 chunkKey = 0;
    if (GET_SINGLE(ChunkManager)->TryGetChunkKey(entity, chunkKey))
        id.bucket = chunkKey;

    return id;
}

void InstancingManager::MarkEntityMeshDirty(Entity* entity)
{
    const InstanceID id = GetMeshInstanceID(entity);
    if (!id.IsValid()) return;
    _partialDirtyMesh.insert(id);
    _dynamicMeshIds.insert(id);
}

InstancingBuffer& InstancingManager::GetOrCreateMeshBuffer(const InstanceID& id)
{
    const bool shouldBeDynamic = (_dynamicMeshIds.count(id) > 0);
    auto& bufPtr = _buffers[id];

    if (!bufPtr)
        bufPtr = std::make_unique<InstancingBuffer>(shouldBeDynamic);
    else if (shouldBeDynamic && !bufPtr->IsDynamic())
        bufPtr->PromoteToDynamic();

    return *bufPtr;
}

void InstancingManager::SmartRebuildMeshGroups(std::vector<Entity*>& entities)
{
    EntityCache newMeshCache;

    for (Entity* entity : entities)
    {
        auto* mr = entity->GetComponent<MeshRenderer>();
        if (!mr) continue;
        const InstanceID id = GetMeshInstanceID(entity);
        newMeshCache[id].push_back(entity);
    }

    for (auto& [id, newVec] : newMeshCache)
    {
        const auto   oldIt   = _meshCache.find(id);
        const uint32 oldSize = (oldIt != _meshCache.end())
                             ? static_cast<uint32>(oldIt->second.size()) : 0u;
        const uint32 newSize = static_cast<uint32>(newVec.size());

        const bool sizeChanged    = (oldSize != newSize);
        const bool transformDirty = (_partialDirtyMesh.count(id) > 0);

        if (!sizeChanged && !transformDirty)
        {
            _stats.meshGroupsSkipped++;
            continue;
        }

        auto& worldVec = _meshWorldCache[id];
        worldVec.resize(newSize);

        uint32 wi = 0;
        for (Entity* entity : newVec)
        {
            RenderPacket packet;
            auto* mr = entity->GetComponent<MeshRenderer>();
            auto* tr = entity->GetComponent<Transform>();
            if (mr && tr && mr->FillPacket(tr->GetWorldMatrix(), packet))
            {
                InstancingData& data = worldVec[wi++];
                data.world           = packet.matWorld;
                data.materialIndex   = packet.materialIndex;
                data._instPad[0]     = 0u;
                data._instPad[1]     = 0u;
                data._instPad[2]     = 0u;
            }
        }
        worldVec.resize(wi);

        InstancingBuffer& buf = GetOrCreateMeshBuffer(id);
        buf.SetData(worldVec.data(), static_cast<uint32>(worldVec.size()));
        buf.UploadData();

        _stats.meshGroupsRebuilt++;
    }

    for (auto& [id, oldVec] : _meshCache)
    {
        if (newMeshCache.count(id) > 0) continue;

        auto bufIt = _buffers.find(id);
        if (bufIt != _buffers.end())
        {
            bufIt->second->ClearData();
            bufIt->second->ResetUpload();
        }
        _meshWorldCache.erase(id);
    }
    _meshCache = std::move(newMeshCache);
    _partialDirtyMesh.clear();
}

void InstancingManager::Render(std::vector<Entity*>& entities)
{
    if (entities.empty()) return;

    GET_SINGLE(DynamicInstancePool)->BeginFrame();

    const bool fullModelRebuild = _bDirty;
    const bool fullMeshRebuild  = _meshDirty;

    if (fullModelRebuild)
    {
        for (auto& [id, _] : _modelCache)
        {
            auto it = _buffers.find(id);
            if (it != _buffers.end()) it->second->ClearData();
        }
        for (auto& [id, _] : _animCache)
        {
            auto it = _buffers.find(id);
            if (it != _buffers.end()) it->second->ClearData();
        }
        _modelCache.clear();
        _modelWorldCache.clear();
        _animCache.clear();
    }

    if (fullMeshRebuild)
    {
        _meshCache.clear();
        _meshWorldCache.clear();
    }

    for (Entity* entity : entities)
    {
        if (auto* mr = entity->GetComponent<MeshRenderer>())
        {
            if (!fullMeshRebuild) continue;

            const InstanceID id = GetMeshInstanceID(entity);
            _meshCache[id].push_back(entity);

            RenderPacket packet;
            auto* tr = entity->GetComponent<Transform>();
            if (tr && mr->FillPacket(tr->GetWorldMatrix(), packet))
            {
                InstancingData data{};
                data.world         = packet.matWorld;
                data.materialIndex = packet.materialIndex;
                data._instPad[0]   = 0u;
                data._instPad[1]   = 0u;
                data._instPad[2]   = 0u;
                _meshWorldCache[id].push_back(data);
            }
        }
        else if (fullModelRebuild)
        {
            if (auto* modelR = entity->GetComponent<ModelRenderer>())
            {
                const InstanceID id = modelR->GetInstanceID();
                InstancingData data{};
                data.world         = modelR->GetModelScaleMatrix()
                                   * entity->GetComponent<Transform>()->GetWorldMatrix();
                data.materialIndex = 0u;
                data._instPad[0]   = 0u;
                data._instPad[1]   = 0u;
                data._instPad[2]   = 0u;
                _modelCache[id].push_back(entity);
                _modelWorldCache[id].push_back(data);
            }
            else if (auto* anim = entity->GetComponent<ModelAnimator>())
            {
                const InstanceID id = anim->GetInstanceID();
                _animCache[id].push_back(entity);
            }
        }
    }

    _stats = RenderStats{};

    if (fullMeshRebuild)
    {
        for (auto& [id, dataVec] : _meshWorldCache)
        {
            InstancingBuffer& buf = GetOrCreateMeshBuffer(id);
            buf.SetData(dataVec.data(), static_cast<uint32>(dataVec.size()));
            buf.UploadData();
        }
        _meshDirty      = false;
        _meshGroupDirty = false;
        _partialDirtyMesh.clear();
    }
    else if (_meshGroupDirty)
    {
        SmartRebuildMeshGroups(entities);
        _meshGroupDirty = false;
    }
    else if (!_partialDirtyMesh.empty())
    {
        for (const InstanceID& dirtyId : _partialDirtyMesh)
        {
            const auto cacheIt = _meshCache.find(dirtyId);
            if (cacheIt == _meshCache.end()) continue;

            auto& worldVec = _meshWorldCache[dirtyId];
            worldVec.resize(cacheIt->second.size());

            uint32 wi = 0;
            for (Entity* entity : cacheIt->second)
            {
                RenderPacket packet;
                auto* mr = entity->GetComponent<MeshRenderer>();
                auto* tr = entity->GetComponent<Transform>();
                if (mr && tr && mr->FillPacket(tr->GetWorldMatrix(), packet))
                {
                    InstancingData& data = worldVec[wi++];
                    data.world           = packet.matWorld;
                    data.materialIndex   = packet.materialIndex;
                    data._instPad[0]     = 0u;
                    data._instPad[1]     = 0u;
                    data._instPad[2]     = 0u;
                }
            }
            worldVec.resize(wi);

            InstancingBuffer& buf = GetOrCreateMeshBuffer(dirtyId);
            buf.SetData(worldVec.data(), static_cast<uint32>(worldVec.size()));
            buf.UploadData();
        }
        _partialDirtyMesh.clear();
    }

    if (fullModelRebuild)
    {
        for (auto& [id, dataVec] : _modelWorldCache)
        {
            auto& bufPtr = _buffers[id];
            if (!bufPtr)
                bufPtr = std::make_unique<InstancingBuffer>(false);
            bufPtr->SetData(dataVec.data(), static_cast<uint32>(dataVec.size()));
            bufPtr->UploadData();
        }
        _bDirty = false;
        _partialDirtyModel.clear();
    }
    else if (!_partialDirtyModel.empty())
    {
        for (const InstanceID& dirtyId : _partialDirtyModel)
        {
            const auto cacheIt = _modelCache.find(dirtyId);
            if (cacheIt == _modelCache.end()) continue;

            auto& worldVec = _modelWorldCache[dirtyId];
            worldVec.resize(cacheIt->second.size());

            uint32 wi = 0;
            for (Entity* entity : cacheIt->second)
            {
                if (auto* modelR = entity->GetComponent<ModelRenderer>())
                {
                    InstancingData& data = worldVec[wi++];
                    data.world           = modelR->GetModelScaleMatrix()
                                         * entity->GetComponent<Transform>()->GetWorldMatrix();
                    data.materialIndex   = 0u;
                    data._instPad[0]     = 0u;
                    data._instPad[1]     = 0u;
                    data._instPad[2]     = 0u;
                }
            }
            worldVec.resize(wi);

            auto& bufPtr = _buffers[dirtyId];
            if (!bufPtr)
                bufPtr = std::make_unique<InstancingBuffer>(false);
            bufPtr->SetData(worldVec.data(), static_cast<uint32>(worldVec.size()));
            bufPtr->UploadData();
        }
        _partialDirtyModel.clear();
    }

    RenderMeshRenderer();
    RenderModelRenderer();
    RenderAnimRenderer();

    _stats.totalDrawCalls = _stats.modelDrawCalls + _stats.meshDrawCalls;

    GET_SINGLE(DynamicInstancePool)->EndFrame();

    PruneEmptyGroups();
}

void InstancingManager::PruneEmptyGroups()
{
    for (auto it = _modelCache.begin(); it != _modelCache.end();)
    {
        if (it->second.empty())
        {
            _buffers.erase(it->first);
            _modelWorldCache.erase(it->first);
            it = _modelCache.erase(it);
        }
        else { ++it; }
    }

    for (auto it = _meshCache.begin(); it != _meshCache.end();)
    {
        if (it->second.empty())
        {
            _buffers.erase(it->first);
            _meshWorldCache.erase(it->first);
            it = _meshCache.erase(it);
        }
        else { ++it; }
    }
}

void InstancingManager::ClearData()
{
    for (auto& [id, buf] : _buffers)
        buf->ClearData();
}

void InstancingManager::RenderMeshRenderer()
{
    for (auto& [id, entityVec] : _meshCache)
    {
        if (entityVec.empty()) continue;

        auto it = _buffers.find(id);
        if (it == _buffers.end() || !it->second->IsUploaded()) continue;

        it->second->PushData();
        entityVec[0]->GetComponent<MeshRenderer>()->RenderInstancing(it->second.get());

        _stats.meshDrawCalls++;
        _stats.totalInstances += it->second->GetCount();

        if (it->second->IsDynamic()) _stats.dynamicBuffers++;
        else                         _stats.staticBuffers++;
    }
}

void InstancingManager::RenderModelRenderer()
{
    for (auto& [id, entityVec] : _modelCache)
    {
        if (entityVec.empty()) continue;

        auto it = _buffers.find(id);
        if (it == _buffers.end() || !it->second->IsUploaded()) continue;

        it->second->PushData();
        entityVec[0]->GetComponent<ModelRenderer>()->RenderInstancing(it->second.get());

        _stats.modelDrawCalls++;
        _stats.totalInstances += it->second->GetCount();
    }
}

void InstancingManager::RenderAnimRenderer()
{
    for (auto& [id, entityVec] : _animCache)
    {
        if (entityVec.empty()) continue;

        auto it = _buffers.find(id);
        if (it != _buffers.end())
            it->second->ClearData();

        InstancedTweenDesc tweenDesc;
        for (int32 i = 0; i < static_cast<int32>(entityVec.size()); i++)
        {
            Entity* entity = entityVec[i];
            auto*   anim   = entity->GetComponent<ModelAnimator>();
            auto*   tr     = entity->GetComponent<Transform>();

            InstancingData data{};
            data.world         = tr->GetWorldMatrix();
            data.materialIndex = 0u;
            data._instPad[0]   = 0u;
            data._instPad[1]   = 0u;
            data._instPad[2]   = 0u;

            AddData(id, data, true);

            anim->UpdateTweenData();
            tweenDesc.tweens[i] = anim->GetTweenDesc();
        }

        auto* anim = entityVec[0]->GetComponent<ModelAnimator>();
        anim->GetShader()->PushTweenData(tweenDesc);

        InstancingBuffer* buffer = _buffers[id].get();
        anim->RenderInstancing(buffer);
    }
}

void InstancingManager::AddData(InstanceID instanceID, const InstancingData& data, bool isDynamic)
{
    auto& bufPtr = _buffers[instanceID];
    if (!bufPtr)
        bufPtr = std::make_unique<InstancingBuffer>(isDynamic);
    bufPtr->AddData(data);
}

void InstancingManager::DumpInstancingStats() const
{
    ::OutputDebugStringW(L"\n========== [InstancingManager Stats] ==========\n");

    wchar_t buf[1024];

    size_t totalMesh = 0;
    for (const auto& [id, v] : _meshCache)  totalMesh  += v.size();
    size_t totalModel = 0;
    for (const auto& [id, v] : _modelCache) totalModel += v.size();

    uint32 dynamicCount = 0, staticCount = 0;
    for (const auto& [id, bufPtr] : _buffers)
        if (bufPtr) { if (bufPtr->IsDynamic()) ++dynamicCount; else ++staticCount; }

    swprintf_s(buf,
        L"[Pool] Dynamic(DISCARD x1 + NO_OVERWRITE x%u) / Static(UpdateSubresource x%u)\n"
        L"[SmartRebuild] rebuilt=%u  skipped=%u  skipRate=%.1f%%\n"
        L"[DrawCall] Mesh=%zu  Model=%zu  Total=%zu\n"
        L"[Instance] Mesh=%zu -> %zu DC (%.1f%% saved)\n"
        L"           Model=%zu -> %zu DC\n"
        L"================================================\n",
        dynamicCount > 0 ? dynamicCount - 1 : 0,
        staticCount,
        _stats.meshGroupsRebuilt, _stats.meshGroupsSkipped,
        (_stats.meshGroupsRebuilt + _stats.meshGroupsSkipped > 0
            ? static_cast<double>(_stats.meshGroupsSkipped)
              / (_stats.meshGroupsRebuilt + _stats.meshGroupsSkipped) * 100.0
            : 0.0),
        _meshCache.size(), _modelCache.size(), _meshCache.size() + _modelCache.size(),
        totalMesh, _meshCache.size(),
        (totalMesh > 0
            ? (1.0 - static_cast<double>(_meshCache.size()) / totalMesh) * 100.0
            : 0.0),
        totalModel, _modelCache.size());

    ::OutputDebugStringW(buf);
}
