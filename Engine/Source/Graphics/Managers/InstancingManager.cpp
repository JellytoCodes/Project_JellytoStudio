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
				const InstanceID instanceID = entity->GetComponent<ModelRenderer>()->GetInstanceID();
				_modelCache[instanceID].push_back(entity);
			}
			else if (entity->GetComponent<ModelAnimator>() != nullptr)
			{
				const InstanceID instanceID = entity->GetComponent<ModelAnimator>()->GetInstanceID();
				_animCache[instanceID].push_back(entity);
			}
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
	for (auto& pair : _modelCache)
	{
		if (pair.second.empty()) continue;
		const InstanceID id = pair.first;

		if (_buffers.find(id) != _buffers.end())
			_buffers[id]->ClearData();

		for (int32 i = 0; i < pair.second.size(); i++)
		{
			const std::shared_ptr<Entity>& entity = pair.second[i];
			InstancingData data;
			auto mr = entity->GetComponent<ModelRenderer>();
			Matrix scaleMatrix = mr ? mr->GetModelScaleMatrix() : Matrix::Identity;
			data.world = scaleMatrix * entity->GetTransform()->GetWorldMatrix();

			AddData(id, data);
		}

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