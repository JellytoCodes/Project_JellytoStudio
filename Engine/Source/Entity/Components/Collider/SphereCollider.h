#pragma once
#include "BaseCollider.h"

class SphereCollider : public BaseCollider
{
	using Super = BaseCollider;

public:
	SphereCollider();
	virtual ~SphereCollider();

	bool Intersects(Ray& ray, OUT float& distance) override;
	bool Intersects(std::shared_ptr<BaseCollider>& other) override;

	BoundingSphere& GetBoundingSphere() { return _boundingSphere; }

	// 기본 반지름 (offsetScale 적용 전)
	float GetRadius() const { return _radius; }
	void SetRadius(float r) { _radius = r; }

protected:
	void UpdateBounds() override;

private:
	float			_radius = 0.5f;
	BoundingSphere	_boundingSphere = {};
};