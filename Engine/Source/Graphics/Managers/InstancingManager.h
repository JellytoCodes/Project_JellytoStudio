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

	// dirty 시점에 계산된 world matrix 캐시
	// 정적 블록은 이후 매프레임 GetComponent + 행렬곱 없이 바로 사용
	std::map<InstanceID, std::vector<InstancingData>> _modelWorldCache;

	bool _bDirty = true;
};