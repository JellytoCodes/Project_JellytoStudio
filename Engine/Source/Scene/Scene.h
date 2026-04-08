#pragma once
#include "Entity/Components/Collider/CollisionChannel.h"

class Light;
class Camera;
class Entity;

class Scene
{
public:
	Scene();
	virtual ~Scene();

	void SetName(const std::wstring& name) { _name = name; }
	const std::wstring& GetName() const { return _name; }

	virtual void Awake();
	virtual void Start();
	virtual void Update();
	virtual void LateUpdate();
	virtual void OnDestroy();

	virtual void Render();

	virtual void Add(std::unique_ptr<Entity> object);
	virtual void Remove(Entity* object);

	void							SetMainCamera(Camera* mainCamera) { _mainCamera = mainCamera; }
	Camera*							GetMainCamera() { return _mainCamera; }

	Light* GetLight() { return _mainLight; }
	void SetMainLight(Light* light) { _mainLight = light; }

	std::shared_ptr<Entity> Pick(int32 screenX, int32 screenY);

	bool PickBlock(int32 screenX, int32 screenY, CollisionChannel queryChan,
		Entity* outEntity,Vec3& outHitNormal,float& outDist);

	bool PickGroundPoint(int32 screenX, int32 screenY, Vec3& outWorldPos, float groundY = 0.f);

	std::unordered_set<std::shared_ptr<Entity>>& GetEntities() { return _objects; }

	// 콜라이더 보유 Entity 캐시 — CollisionManager가 O(N²)→O(C²)에 활용
	std::unordered_set<std::shared_ptr<Entity>>& GetCollidableEntities() { return _collidableObjects; }

private:
	std::wstring									_name = L"Unnamed Scene";
	std::unordered_set<std::unique_ptr<Entity>>		_objects;
	Camera*											_mainCamera;
	Light*											_mainLight;

	// ── Render 루프 캐시 ─────────────────────────────────────────
	std::unordered_set<Entity*>		_collidableObjects;
	std::vector<Entity*>			_widgetObjects;
};