#pragma once
#include "BaseCollider.h"

class OBBCollider : public BaseCollider
{
	using Super = BaseCollider;

public:
	OBBCollider();
	virtual ~OBBCollider();

	bool Intersects(Ray& ray, float& distance) override;
	bool Intersects(std::shared_ptr<BaseCollider>& other) override;

	BoundingOrientedBox& GetBoundingBox() { return _boundingBox; }

private:
	BoundingOrientedBox _boundingBox;
};