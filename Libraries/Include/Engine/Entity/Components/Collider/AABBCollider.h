#pragma once
#include "BaseCollider.h"

class AABBCollider : public BaseCollider
{
	using Super = BaseCollider;

public:
	AABBCollider();
	virtual ~AABBCollider();

	bool Intersects(Ray& ray, float& distance) override;
	bool Intersects(std::shared_ptr<BaseCollider>& other) override;

	BoundingBox& GetBoundingBox() { return _boundingBox; }

private :
	BoundingBox _boundingBox;
};
