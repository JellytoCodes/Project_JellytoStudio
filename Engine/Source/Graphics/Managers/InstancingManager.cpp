
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

    if (::GetAsyncKeyState(VK_F1) & 0x8000)
    {
        static bool s_dumped = false;
        if (!s_dumped) { DumpInstancingStats(); s_dumped = true; }
    }
    else { static bool s_dumped = false; s_dumped = false; }

    if (_bDirty)
    {
        ClearData();
        _meshCache.clear();
        _modelCache.clear();
        _modelWorldCache.clear();
        _animCache.clear();

        for (Entity* entity : entities)
        {
            if (auto* mr = entity->GetComponent<MeshRenderer>())
            {
                const InstanceID instanceID = mr->GetInstanceID();
                _meshCache[instanceID].push_back(entity);
            }
            else if (auto* modelR = entity->GetComponent<ModelRenderer>())
            {
                const InstanceID instanceID = modelR->GetInstanceID();
                InstancingData data;
                data.world = modelR->GetModelScaleMatrix()
                           * entity->GetComponent<Transform>()->GetWorldMatrix();
                _modelCache[instanceID].push_back(entity);
                _modelWorldCache[instanceID].push_back(data);
            }
            else if (auto* anim = entity->GetComponent<ModelAnimator>())
            {
                const InstanceID instanceID = anim->GetInstanceID();
                _animCache[instanceID].push_back(entity);
            }
        }

        for (auto& [id, dataVec] : _modelWorldCache)
        {
            auto it = _buffers.find(id);
            if (it == _buffers.end())
            {
                _buffers[id] = std::make_unique<InstancingBuffer>();
                it = _buffers.find(id);
            }
            else
            {
                it->second->ClearData();
            }
            for (const InstancingData& d : dataVec)
                AddData(id, d);
            it->second->UploadData();
        }

        _bDirty = false;
    }

    RenderMeshRenderer();
    RenderModelRenderer();
    RenderAnimRenderer();
}

void InstancingManager::ClearData()
{
    for (auto& [id, buffer] : _buffers)
        buffer->ClearData();
}

void InstancingManager::RenderMeshRenderer()
{
    for (auto& [id, entityVec] : _meshCache)
    {
        if (entityVec.empty()) continue;

        auto it = _buffers.find(id);
        if (it != _buffers.end())
            it->second->ClearData();

        for (Entity* entity : entityVec)
        {
            auto* mr = entity->GetComponent<MeshRenderer>();
            auto* tr = entity->GetComponent<Transform>();

            if (mr == nullptr || tr == nullptr) continue;

            RenderPacket packet;
            if (!mr->FillPacket(tr->GetWorldMatrix(), packet))
                continue;

            InstancingData data;
            data.world = packet.matWorld;
            AddData(packet.instanceID, data);
        }

        InstancingBuffer* buffer = _buffers[id].get();
        if (buffer == nullptr || buffer->GetCount() == 0) continue;

        entityVec[0]->GetComponent<MeshRenderer>()->RenderInstancing(buffer);
    }
}

void InstancingManager::RenderModelRenderer()
{
    for (auto& [id, entityVec] : _modelCache)
    {
        if (entityVec.empty()) continue;

        auto it = _buffers.find(id);
        if (it == _buffers.end()) continue;
        if (!it->second->IsUploaded()) continue;

        InstancingBuffer* buffer = it->second.get();
        entityVec[0]->GetComponent<ModelRenderer>()->RenderInstancing(buffer);
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

            InstancingData data;
            data.world = entity->GetComponent<Transform>()->GetWorldMatrix();
            AddData(id, data);

            auto* anim = entity->GetComponent<ModelAnimator>();
            anim->UpdateTweenData();
            tweenDesc.tweens[i] = anim->GetTweenDesc();
        }

        auto* anim = entityVec[0]->GetComponent<ModelAnimator>();
        anim->GetShader()->PushTweenData(tweenDesc);

        InstancingBuffer* buffer = _buffers[id].get();
        anim->RenderInstancing(buffer);
    }
}

void InstancingManager::AddData(InstanceID instanceID, const InstancingData& data)
{
    auto it = _buffers.find(instanceID);
    if (it == _buffers.end())
    {
        _buffers[instanceID] = std::make_unique<InstancingBuffer>();
        it = _buffers.find(instanceID);
    }
    it->second->AddData(data);
}

void InstancingManager::DumpInstancingStats() const
{
    ::OutputDebugStringW(L"\n========== [InstancingManager 진단] ==========\n");

    wchar_t buf[512];
    swprintf_s(buf, L"ModelRenderer 그룹 수 (= DrawCall 수): %zu\n", _modelCache.size());
    ::OutputDebugStringW(buf);

    int groupIdx = 0;
    for (const auto& [id, entityVec] : _modelCache)
    {
        bool uploaded = false;
        auto bufIt = _buffers.find(id);
        if (bufIt != _buffers.end())
            uploaded = bufIt->second->IsUploaded();

        swprintf_s(buf,
            L"  [그룹 %d] model=%llx shader=%llx | 인스턴스=%zu | 업로드=%s\n",
            groupIdx++, id.first, id.second,
            entityVec.size(),
            uploaded ? L"OK" : L"X");
        ::OutputDebugStringW(buf);
    }

    swprintf_s(buf, L"MeshRenderer 그룹 수: %zu\n", _meshCache.size());
    ::OutputDebugStringW(buf);
    for (const auto& [id, entityVec] : _meshCache)
    {
        swprintf_s(buf, L"  mesh=%llx mat=%llx | 인스턴스=%zu\n",
            id.first, id.second, entityVec.size());
        ::OutputDebugStringW(buf);
    }

    size_t totalModel = 0;
    for (const auto& [id, v] : _modelCache) totalModel += v.size();
    size_t totalMesh = 0;
    for (const auto& [id, v] : _meshCache)  totalMesh  += v.size();

    swprintf_s(buf,
        L"총 렌더 Entity: ModelRenderer=%zu MeshRenderer=%zu\n"
        L"총 DrawCall (서브메시 제외): ModelGroup=%zu + MeshGroup=%zu\n"
        L"인스턴싱 효율: %.1f%% (%zu Entity → %zu 그룹)\n"
        L"============================================\n",
        totalModel, totalMesh,
        _modelCache.size(), _meshCache.size(), 
        (totalModel > 0 ? (1.0 - static_cast<double>(_modelCache.size()) / totalModel) * 100.0 : 0.0), 
        totalModel, 
        _modelCache.size());
    ::OutputDebugStringW(buf);
}
