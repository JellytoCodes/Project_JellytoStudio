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

	std::shared_ptr<Light>							GetLight()													{ return _mainLight;  }

	// 스크린 좌표 기반 피킹 ? View Space에서 Ray 생성 후 World 변환
	std::shared_ptr<Entity> Pick(int32 screenX, int32 screenY);

	std::unordered_set<std::shared_ptr<Entity>>&	GetEntities()												{ return _objects; }

private :
	std::wstring									_name = L"Unnamed Scene";
	std::unordered_set<std::shared_ptr<Entity>>		_objects;
	std::shared_ptr<Camera>							_mainCamera;
	std::shared_ptr<Light>							_mainLight;
};