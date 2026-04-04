#pragma once
#include "Entity/Entity.h"
#include "Pipeline/InstancingBuffer.h"

class InstancingManager
{
	DECLARE_SINGLE(InstancingManager);

public:
	void Render(std::vector<std::shared_ptr<Entity>>& Entities);
	void ClearData();

	void SetDirty() { _bDirty = true; }

private:
	void RenderMeshRenderer();
	void RenderModelRenderer();
	void RenderAnimRenderer();

	void AddData(InstanceID instanceID, InstancingData& data);

	std::map<InstanceID, std::shared_ptr<InstancingBuffer>> _buffers;

	std::map<InstanceID, std::vector<std::shared_ptr<Entity>>> _meshCache;
	std::map<InstanceID, std::vector<std::shared_ptr<Entity>>> _modelCache;
	std::map<InstanceID, std::vector<std::shared_ptr<Entity>>> _animCache;

	bool _bDirty = true;
};
