#pragma once
#include "BaseCollider.h"

class SphereCollider : public BaseCollider
{
	using Super = BaseCollider;

public:
	SphereCollider();
	virtual ~SphereCollider();

	bool Intersects(Ray& ray, float& distance) override;
	bool Intersects(std::shared_ptr<BaseCollider>& other) override;

	BoundingSphere& GetBoundingSphere() { return _boundingSphere; }

private:
	BoundingSphere _boundingSphere;
};