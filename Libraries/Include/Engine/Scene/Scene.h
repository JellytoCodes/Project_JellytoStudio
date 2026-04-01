#pragma once
#include "Entity/Components/Collider/CollisionChannel.h"

class Light;
class Camera;
class Entity;

class Scene
{
public :
	Scene();
	virtual ~Scene();

	void SetName(const std::wstring& name) { _name = name; }
	const std::wstring& GetName() const    { return _name; }

	virtual void Awake();
	virtual void Start();
	virtual void Update();
	virtual void LateUpdate();
	virtual void OnDestroy();

	virtual void Render();

	virtual void Add(const std::shared_ptr<Entity>& object);
	virtual void Remove(const std::shared_ptr<Entity>& object);

	void											SetMainCamera(const std::shared_ptr<Camera>& mainCamera)	{ _mainCamera = mainCamera; }
	std::shared_ptr<Camera>							GetMainCamera()												{ return _mainCamera; }

	std::shared_ptr<Light> GetLight();
	void SetMainLight(const std::shared_ptr<Light>& light);

	std::shared_ptr<Entity> Pick(int32 screenX, int32 screenY);

	// 채널 필터링 + 히트 노말 반환 버전
	// queryChan: 이 채널로 피킹 — target.CanBePickedBy(queryChan) 검사
	// outHitNormal: 히트된 AABB 면의 노말 벡터 (±X/Y/Z)
	bool PickBlock(int32 screenX, int32 screenY,
	               CollisionChannel queryChan,
	               std::shared_ptr<Entity>& outEntity,
	               Vec3& outHitNormal,
	               float& outDist);

	bool PickGroundPoint(int32 screenX, int32 screenY, Vec3& outWorldPos, float groundY = 0.f);

	std::unordered_set<std::shared_ptr<Entity>>&	GetEntities()												{ return _objects; }

private :
	std::wstring									_name = L"Unnamed Scene";
	std::unordered_set<std::shared_ptr<Entity>>		_objects;
	std::shared_ptr<Camera>							_mainCamera;
	std::shared_ptr<Light>							_mainLight;
};