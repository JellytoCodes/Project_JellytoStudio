#include "Framework.h"
#include "InstancingManager.h"

#include "Entity/Entity.h"
#include "Entity/Components/MeshRenderer.h"
#include "Graphics/Model/ModelRenderer.h"
#include "Graphics/Model/ModelAnimator.h"
#include "Pipeline/Shader.h"

void InstancingManager::Render(std::vector<std::shared_ptr<Entity>>& Entities)
{
	if (Entities.empty()) return;

	if (_bDirty)
	{
		ClearData();
		_meshCache.clear();
		_modelCache.clear();
		_modelWorldCache.clear();
		_animCache.clear();

		for (std::shared_ptr<Entity>& entity : Entities)
		{
			if (entity->GetComponent<MeshRenderer>() != nullptr)
			{
				const InstanceID instanceID = entity->GetComponent<MeshRenderer>()->GetInstanceID();
				_meshCache[instanceID].push_back(entity);
			}
			else if (entity->GetComponent<ModelRenderer>() != nullptr)
			{
				auto mr = entity->GetComponent<ModelRenderer>();
				const InstanceID instanceID = mr->GetInstanceID();
				// world matrix를 dirty 시점에 미리 계산해서 캐시
				// 정적 블록은 이후 매프레임 GetComponent+행렬곱 필요 없음
				Matrix scaleMatrix = mr->GetModelScaleMatrix();
				InstancingData data;
				data.world = scaleMatrix * entity->GetTransform()->GetWorldMatrix();
				_modelCache[instanceID].push_back(entity);
				_modelWorldCache[instanceID].push_back(data);
			}
			else if (entity->GetComponent<ModelAnimator>() != nullptr)
			{
				const InstanceID instanceID = entity->GetComponent<ModelAnimator>()->GetInstanceID();
				_animCache[instanceID].push_back(entity);
			}
		}

		// worldCache → InstancingBuffer 적재 + GPU 업로드 (dirty 시 1회)
		// 이후 매프레임 RenderModelRenderer는 BindBuffer + Draw만 실행
		for (auto& pair : _modelWorldCache)
		{
			const InstanceID id = pair.first;
			if (_buffers.find(id) == _buffers.end())
				_buffers[id] = std::make_shared<InstancingBuffer>();
			else
				_buffers[id]->ClearData();
			for (const InstancingData& data : pair.second)
				AddData(id, const_cast<InstancingData&>(data));
			_buffers[id]->UploadData(); // GPU 업로드 — dirty 시 1회만
		}

		_bDirty = false;
	}

	RenderMeshRenderer();
	RenderModelRenderer();
	RenderAnimRenderer();
}

void InstancingManager::ClearData()
{
	for (auto& pair : _buffers)
	{
		std::shared_ptr<InstancingBuffer>& buffer = pair.second;
		buffer->ClearData();
	}
}

void InstancingManager::RenderMeshRenderer()
{
	for (auto& pair : _meshCache)
	{
		if (pair.second.empty()) continue;
		const InstanceID id = pair.first;

		// 매프레임 world matrix 갱신 — MeshRenderer도 ModelRenderer와 동일하게 처리
		// (프리뷰 큐브처럼 동적으로 움직이는 MeshRenderer가 freeze되는 버그 수정)
		if (_buffers.find(id) != _buffers.end())
			_buffers[id]->ClearData();

		for (const std::shared_ptr<Entity>& entity : pair.second)
		{
			InstancingData data;
			data.world = entity->GetTransform()->GetWorldMatrix();
			AddData(id, data);
		}

		std::shared_ptr<InstancingBuffer>& buffer = _buffers[id];
		pair.second[0]->GetComponent<MeshRenderer>()->RenderInstancing(buffer);
	}
}

void InstancingManager::RenderModelRenderer()
{
	// ── 매프레임 UploadData / ClearData / AddData 호출 없음 ──────
	// dirty 시점에 이미 조립+업로드 완료 → 여기서는 BindBuffer+Draw만
	for (auto& pair : _modelCache)
	{
		if (pair.second.empty()) continue;
		const InstanceID id = pair.first;

		if (_buffers.find(id) == _buffers.end()) continue;
		if (!_buffers[id]->IsUploaded()) continue; // 아직 업로드 안 됐으면 스킵

		std::shared_ptr<InstancingBuffer>& buffer = _buffers[id];
		pair.second[0]->GetComponent<ModelRenderer>()->RenderInstancing(buffer);
	}
}

void InstancingManager::RenderAnimRenderer()
{
	for (auto& pair : _animCache)
	{
		if (pair.second.empty()) continue;
		const InstanceID id = pair.first;

		if (_buffers.find(id) != _buffers.end())
			_buffers[id]->ClearData();

		std::shared_ptr<InstancedTweenDesc> tweenDesc = std::make_shared<InstancedTweenDesc>();

		for (int32 i = 0; i < pair.second.size(); i++)
		{
			const std::shared_ptr<Entity>& entity = pair.second[i];
			InstancingData data;
			data.world = entity->GetTransform()->GetWorldMatrix();

			AddData(id, data);

			entity->GetComponent<ModelAnimator>()->UpdateTweenData();
			tweenDesc->tweens[i] = entity->GetComponent<ModelAnimator>()->GetTweenDesc();
		}

		pair.second[0]->GetComponent<ModelAnimator>()->GetShader()->PushTweenData(*tweenDesc.get());

		std::shared_ptr<InstancingBuffer>& buffer = _buffers[id];
		pair.second[0]->GetComponent<ModelAnimator>()->RenderInstancing(buffer);
	}
}

void InstancingManager::AddData(InstanceID instanceID, InstancingData& data)
{
	if (_buffers.find(instanceID) == _buffers.end())
		_buffers[instanceID] = std::make_shared<InstancingBuffer>();

	_buffers[instanceID]->AddData(data);
}