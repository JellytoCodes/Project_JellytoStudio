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

	// 박스 반지름 (각 축 ±Extents 크기, offsetScale 적용 전)
	// 예) SetBoxExtents({0.5f, 1.f, 0.5f}) → X/Z는 ±0.5, Y는 ±1 범위
	Vec3 GetBoxExtents() const { return _boxExtents; }
	void SetBoxExtents(const Vec3& e) { _boxExtents = e; }

protected:
	void UpdateBounds() override;

private:
	Vec3		_boxExtents = { 0.5f, 0.5f, 0.5f };
	BoundingBox	_boundingBox = {};
};