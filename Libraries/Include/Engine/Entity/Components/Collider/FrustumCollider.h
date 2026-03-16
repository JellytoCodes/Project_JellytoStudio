#pragma once
#include "BaseCollider.h"

class FrustumCollider : public BaseCollider
{
	using Super = BaseCollider;

public:
	FrustumCollider();
	virtual ~FrustumCollider();

	bool Intersects(Ray& ray, float& distance) override;
	bool Intersects(std::shared_ptr<BaseCollider>& other) override;

	BoundingFrustum& GetBoundingFrustum() { return _boundingFrustum; }

private:
	BoundingFrustum _boundingFrustum;
};
