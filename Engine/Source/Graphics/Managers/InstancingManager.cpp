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

	ClearData();

	RenderMeshRenderer(Entities);
	RenderModelRenderer(Entities);
	//RenderAnimRenderer(Entities);
}

void InstancingManager::ClearData()
{
	for (auto& pair : _buffers)
	{
		std::shared_ptr<InstancingBuffer>& buffer = pair.second;
		buffer->ClearData();
	}
}

void InstancingManager::RenderMeshRenderer(std::vector<std::shared_ptr<Entity>>& Entities)
{
	std::map<InstanceID, std::vector<std::shared_ptr<Entity>>> cache;

	for (std::shared_ptr<Entity>& entity : Entities)
	{
		if (entity->GetComponent<MeshRenderer>() == nullptr) continue;

		const InstanceID instanceID = entity->GetComponent<MeshRenderer>()->GetInstanceID();
		cache[instanceID].push_back(entity);
	}

	for (auto& pair : cache)
	{
		const std::vector<std::shared_ptr<Entity>>& vec = pair.second;
		{
			const InstanceID id = pair.first;

			for (int32 i = 0; i < vec.size(); i++)
			{
				const std::shared_ptr<Entity>& entity = vec[i];
				InstancingData data;
				data.world = entity->GetTransform()->GetWorldMatrix();

				AddData(id, data);
			}

			std::shared_ptr<InstancingBuffer>& buffer = _buffers[id];
			vec[0]->GetComponent<MeshRenderer>()->RenderInstancing(buffer);
		}
	}
}

void InstancingManager::RenderModelRenderer(std::vector<std::shared_ptr<Entity>>& Entities)
{
	std::map < InstanceID, std::vector<std::shared_ptr<Entity>> > cache;

	for (std::shared_ptr<Entity>& entity : Entities)
	{
		if (entity->GetComponent<ModelRenderer>() == nullptr) continue;

		const InstanceID instanceID = entity->GetComponent<ModelRenderer>()->GetInstanceID();
		cache[instanceID].push_back(entity);
	}

	for (auto& pair : cache)
	{
		const std::vector<std::shared_ptr<Entity>>& vec = pair.second;
		{
			const InstanceID id = pair.first;

			for (int32 i = 0; i < vec.size(); i++)
			{
				const std::shared_ptr<Entity>& entity = vec[i];
				InstancingData data;
				data.world = entity->GetTransform()->GetWorldMatrix();

				AddData(id, data);
			}

			std::shared_ptr<InstancingBuffer>& buffer = _buffers[id];
			vec[0]->GetComponent<ModelRenderer>()->RenderInstancing(buffer);
		}
	}
}

void InstancingManager::RenderAnimRenderer(std::vector<std::shared_ptr<Entity>>& Entities)
{
	std::map <InstanceID, std::vector<std::shared_ptr<Entity>> > cache;

	for (std::shared_ptr<Entity>& entity : Entities)
	{
		if (entity->GetComponent<ModelAnimator>() == nullptr) continue;

		const InstanceID instanceID = entity->GetComponent<ModelAnimator>()->GetInstanceID();
		cache[instanceID].push_back(entity);
	}

	for (auto& pair : cache)
	{
		std::shared_ptr<InstancedTweenDesc> tweenDesc = std::make_shared<InstancedTweenDesc>();

		const std::vector<std::shared_ptr<Entity>>& vec = pair.second;
		{
			const InstanceID id = pair.first;

			for (int32 i = 0; i < vec.size(); i++)
			{
				const std::shared_ptr<Entity>& gameObject = vec[i];
				InstancingData data;
				data.world = gameObject->GetTransform()->GetWorldMatrix();

				AddData(id, data);

				// INSTANCING
				gameObject->GetComponent<ModelAnimator>()->UpdateTweenData();
				tweenDesc->tweens[i] = gameObject->GetComponent<ModelAnimator>()->GetTweenDesc();
			}

			vec[0]->GetComponent<ModelAnimator>()->GetShader()->PushTweenData(*tweenDesc.get());

			std::shared_ptr<InstancingBuffer>& buffer = _buffers[id];
			vec[0]->GetComponent<ModelAnimator>()->RenderInstancing(buffer);
		}
	}
}

void InstancingManager::AddData(InstanceID instanceID, InstancingData& data)
{
	if (_buffers.find(instanceID) == _buffers.end()) _buffers[instanceID] = std::make_shared<InstancingBuffer>();

	_buffers[instanceID]->AddData(data);
}