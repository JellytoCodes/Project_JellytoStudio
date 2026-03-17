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

	// 박스 반지름 (각 축 ±Extents 크기, offsetScale 적용 전)
	Vec3 GetBoxExtents() const { return _boxExtents; }
	void SetBoxExtents(const Vec3& e) { _boxExtents = e; }

protected:
	void UpdateBounds() override;

private:
	Vec3				_boxExtents = { 0.5f, 0.5f, 0.5f };
	BoundingOrientedBox	_boundingBox = {};
};