#include "Framework.h"
#include "InstancingManager.h"
#include "Entity/Entity.h"
#include "Entity/Components/Transform.h"
#include "Entity/Components/MeshRenderer.h"
#include "Graphics/Model/ModelRenderer.h"
#include "Graphics/Model/ModelAnimator.h"
#include "Pipeline/Shader.h"

void InstancingManager::Render(std::vector<Entity*>& entities)
{
    if (entities.empty()) return;

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

            const InstanceID id = mr->GetInstanceID();
            _meshCache[id].push_back(entity);

            RenderPacket packet;
            auto* tr = entity->GetComponent<Transform>();
            if (tr && mr->FillPacket(tr->GetWorldMatrix(), packet))
            {
                InstancingData data;
                data.world = packet.matWorld;
                _meshWorldCache[id].push_back(data);
            }
        }
        else if (fullModelRebuild)
        {
            if (auto* modelR = entity->GetComponent<ModelRenderer>())
            {
                const InstanceID id = modelR->GetInstanceID();
                InstancingData data;
                data.world = modelR->GetModelScaleMatrix()
                    * entity->GetComponent<Transform>()->GetWorldMatrix();

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

    if (fullMeshRebuild)
    {
        for (auto& [id, dataVec] : _meshWorldCache)
        {
            auto& buf = _buffers[id];
            if (!buf)
            {
                buf = std::make_unique<InstancingBuffer>(/*isDynamic=*/false);
            }
            else
            {
                buf->ClearData();
            }

            for (const InstancingData& d : dataVec)
                buf->AddData(d);

            buf->UploadData();
        }

        _meshDirty = false;
        _partialDirtyMesh.clear();
    }
    else if (!_partialDirtyMesh.empty())
    {
        for (const InstanceID& dirtyId : _partialDirtyMesh)
        {
            const auto cacheIt = _meshCache.find(dirtyId);
            if (cacheIt == _meshCache.end()) continue;

            auto& worldVec = _meshWorldCache[dirtyId];
            worldVec.clear();

            for (Entity* entity : cacheIt->second)
            {
                RenderPacket packet;
                auto* mr = entity->GetComponent<MeshRenderer>();
                auto* tr = entity->GetComponent<Transform>();
                if (mr && tr && mr->FillPacket(tr->GetWorldMatrix(), packet))
                {
                    InstancingData data;
                    data.world = packet.matWorld;
                    worldVec.push_back(data);
                }
            }

            auto& buf = _buffers[dirtyId];
            if (!buf)
                buf = std::make_unique<InstancingBuffer>(/*isDynamic=*/false);
            else
                buf->ClearData();

            for (const InstancingData& d : worldVec)
                buf->AddData(d);

            buf->UploadData();
        }

        _partialDirtyMesh.clear();
    }

    if (fullModelRebuild)
    {
        for (auto& [id, dataVec] : _modelWorldCache)
        {
            auto& buf = _buffers[id];
            if (!buf)
                buf = std::make_unique<InstancingBuffer>(/*isDynamic=*/false);
            else
                buf->ClearData();

            for (const InstancingData& d : dataVec)
                buf->AddData(d);

            buf->UploadData();
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
            worldVec.clear();

            for (Entity* entity : cacheIt->second)
            {
                if (auto* modelR = entity->GetComponent<ModelRenderer>())
                {
                    InstancingData data;
                    data.world = modelR->GetModelScaleMatrix()
                        * entity->GetComponent<Transform>()->GetWorldMatrix();
                    worldVec.push_back(data);
                }
            }

            auto& buf = _buffers[dirtyId];
            if (!buf)
                buf = std::make_unique<InstancingBuffer>(/*isDynamic=*/false);
            else
                buf->ClearData();

            for (const InstancingData& d : worldVec)
                buf->AddData(d);

            buf->UploadData();
        }

        _partialDirtyModel.clear();
    }

    _stats = RenderStats{};  // 통계 초기화

    RenderMeshRenderer();
    RenderModelRenderer();
    RenderAnimRenderer();

    _stats.totalDrawCalls = _stats.modelDrawCalls + _stats.meshDrawCalls;
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
            Entity*   entity = entityVec[i];
            auto*     anim   = entity->GetComponent<ModelAnimator>();
            auto*     tr     = entity->GetComponent<Transform>();

            InstancingData data;
            data.world = tr->GetWorldMatrix();

            AddData(id, data, /*isDynamic=*/true);

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
    auto& buf = _buffers[instanceID];
    if (!buf)
        buf = std::make_unique<InstancingBuffer>(isDynamic);

    buf->AddData(data);
}

void InstancingManager::DumpInstancingStats() const
{
    ::OutputDebugStringW(L"\n========== [InstancingManager Phase2 Stats] ==========\n");

    wchar_t buf[512];

    swprintf_s(buf, L"[Phase2] Ring Buffer 슬롯: %u | 정적버퍼 Map 절감: dirty 없는 프레임 Map=0\n",
        InstancingBuffer::kRingCount);
    ::OutputDebugStringW(buf);

    swprintf_s(buf, L"ModelRenderer 그룹 수 (= DrawCall 하한): %zu\n", _modelCache.size());
    ::OutputDebugStringW(buf);

    size_t totalModel = 0;
    for (const auto& [id, v] : _modelCache)
    {
        totalModel += v.size();

        bool uploaded = false;
        const auto it = _buffers.find(id);
        if (it != _buffers.end()) uploaded = it->second->IsUploaded();

        swprintf_s(buf, L"  model=%llx shader=%llx | instances=%zu | uploaded=%s\n",
            id.first, id.second, v.size(), uploaded ? L"OK" : L"X");
        ::OutputDebugStringW(buf);
    }

    size_t totalMesh = 0;
    for (const auto& [id, v] : _meshCache)
        totalMesh += v.size();

    swprintf_s(buf,
        L"MeshRenderer 그룹: %zu | Mesh 인스턴스: %zu\n"
        L"최종 DrawCall: Model=%zu + Mesh=%zu = %zu\n"
        L"인스턴싱 절감률: %.1f%% (%zu Entity → %zu DrawCall)\n"
        L"======================================================\n",
        _meshCache.size(), totalMesh,
        _modelCache.size(), _meshCache.size(), _modelCache.size() + _meshCache.size(),
        (totalModel > 0 ? (1.0 - static_cast<double>(_modelCache.size()) / totalModel) * 100.0 : 0.0),
        totalModel, _modelCache.size());
    ::OutputDebugStringW(buf);
}