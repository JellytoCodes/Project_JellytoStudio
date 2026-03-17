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

	void SetFromProjection(const Matrix& matProjection)
	{
		BoundingFrustum::CreateFromMatrix(_boundingFrustum, matProjection);
	}

protected:
	void UpdateBounds() override;

private:
	BoundingFrustum _boundingFrustum = {};
};