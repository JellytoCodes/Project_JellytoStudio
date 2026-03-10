#pragma once
#include "Entity/Entity.h"

class InstancingManager
{
	DECLARE_SINGLE(InstancingManager);

public:
	void Render(std::vector<std::shared_ptr<Entity>>& Entities);
	void ClearData();

private:
	void RenderMeshRenderer(std::vector<std::shared_ptr<Entity>>& Entities);
	void RenderModelRenderer(std::vector<std::shared_ptr<Entity>>& Entities);
	void RenderAnimRenderer(std::vector<std::shared_ptr<Entity>>& Entities);

	void AddData(InstanceID instanceID, InstancingData& data);


	std::map<InstanceID, std::shared_ptr<InstancingBuffer>> _buffers;
};
