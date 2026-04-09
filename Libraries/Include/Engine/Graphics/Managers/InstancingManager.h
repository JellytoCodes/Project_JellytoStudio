#pragma once
#include "Entity/Entity.h"
#include "Pipeline/InstancingBuffer.h"

class InstancingManager
{
	DECLARE_SINGLE(InstancingManager);

public:
	// ── 렌더 진입점 ────────────────────────────────────────────────────
	// 이전: vector<shared_ptr<Entity>>& → 함수 진입 시 N번 refcount 복사 없음(ref)
	//        내부 캐시 map에 shared_ptr 저장 → 또 N번 복사
	// 변경: vector<Entity*>& → 포인터 배열, refcount 0
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
