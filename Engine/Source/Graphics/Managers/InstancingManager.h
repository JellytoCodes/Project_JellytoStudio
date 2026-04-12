#pragma once
#include "Entity/Entity.h"
#include "Pipeline/InstancingBuffer.h"

class InstancingManager
{
	DECLARE_SINGLE(InstancingManager);

public:
	void Render(std::vector<Entity*>& entities);
	void ClearData();

	void SetDirty() { _bDirty = true; }

	void DumpInstancingStats() const;

private:
	void RenderMeshRenderer();
	void RenderModelRenderer();
	void RenderAnimRenderer();

	void AddData(InstanceID instanceID, const InstancingData& data);

	// ── 버퍼 맵 ─────────────────────────────────────────────────────────
	std::map<InstanceID, std::unique_ptr<InstancingBuffer>> _buffers;

	// ── 엔티티 캐시 ──────────────────────────────────────────────────────
	std::map<InstanceID, std::vector<Entity*>> _meshCache;
	std::map<InstanceID, std::vector<Entity*>> _modelCache;
	std::map<InstanceID, std::vector<Entity*>> _animCache;

	std::map<InstanceID, std::vector<InstancingData>> _modelWorldCache;

	bool _bDirty = true;
};
