#pragma once

class Camera;
class Entity;

class Scene
{
public :
	Scene();
	virtual ~Scene();

	virtual void Awake();
	virtual void Start();
	virtual void Update();
	virtual void LateUpdate();
	virtual void OnDestroy();

	virtual void Render();

	virtual void Add(const std::shared_ptr<Entity>& object);
	virtual void Remove(const std::shared_ptr<Entity>& object);

	unordered_set<std::shared_ptr<Entity>>& GetObjects() { return _objects; }

private :
	unordered_set<std::shared_ptr<Entity>> _objects;

	std::shared_ptr<Camera> _mainCamera;
};
