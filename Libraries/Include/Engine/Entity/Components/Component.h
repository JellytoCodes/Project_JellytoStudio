#pragma once

class Transform;

enum class ComponentType : int8
{
	Transform,
	Light,
	Camera,
	MeshRenderer,
	Collider,
	ModelRenderer,
	Animator,
	AnimStateMachine,
	TileMap,
	UIComponent,

	///////////////
	Script,
	///////////////
	
	End
};

enum { FIXED_COMPONENT_COUNT = static_cast<uint8>(ComponentType::End) - 1 };

class Component
{
	friend class Entity;
	void SetEntity(Entity* entity);

public :
	Component(ComponentType type);
	virtual ~Component();

	virtual void					Awake();
	virtual void					Start();
	virtual void					Update();
	virtual void					LateUpdate();
	virtual void					OnDestroy();

	ComponentType					GetType() const		{ return _type; }

	Entity*			GetEntity();
	Transform*		GetTransform();

protected :
	ComponentType	_type;

	Entity*			_entity;
	Transform*		_transform;
};