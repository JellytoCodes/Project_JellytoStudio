#pragma once

class Transform;

enum class ComponentType : int8
{
	Transform,
	MeshRenderer,
	Camera,


	///////////////
	End
};

enum { FIXED_COMPONENT_COUNT = static_cast<uint8>(ComponentType::End) - 1 };

class Component
{
	friend class Entity;
	void SetEntity(const std::shared_ptr<Entity>& entity);

public :
	Component(ComponentType type);
	virtual ~Component();

	virtual void					Awake();
	virtual void					Start();
	virtual void					Update();
	virtual void					LateUpdate();
	virtual void					OnDestroy();
	virtual void					Render();
	ComponentType					GetType() const		{ return _type; }

	std::shared_ptr<Transform>		GetTransform();
	std::shared_ptr<Entity>			GetEntity();

protected :
	ComponentType _type;

	std::weak_ptr<Entity>		_entity;
	std::shared_ptr<Transform>	_transform;
};