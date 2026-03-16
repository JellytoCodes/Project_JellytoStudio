#pragma once
#include "Entity/Components/Component.h"

enum class ColliderType : uint8
{
	Sphere,
	AABB,
	OBB,
	Frustum,

	End
};

class BaseCollider : Component
{
	using Super = Component;
public:
	BaseCollider(ColliderType colliderType);
	virtual ~BaseCollider();

	virtual bool Intersects(Ray& ray, OUT float& distance) = 0;
	virtual bool Intersects(std::shared_ptr<BaseCollider>& other) = 0;

	ColliderType GetColliderType() const { return _colliderType; }

protected :
	ColliderType _colliderType;
};
