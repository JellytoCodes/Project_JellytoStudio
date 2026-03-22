#pragma once

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

	bool PickGroundPoint(int32 screenX, int32 screenY, Vec3& outWorldPos, float groundY = 0.f);

	std::unordered_set<std::shared_ptr<Entity>>&	GetEntities()												{ return _objects; }

private :
	std::wstring									_name = L"Unnamed Scene";
	std::unordered_set<std::shared_ptr<Entity>>		_objects;
	std::shared_ptr<Camera>							_mainCamera;
	std::shared_ptr<Light>							_mainLight;
};