#pragma once
#include "BaseCollider.h"

class OBBCollider : public BaseCollider
{
	using Super = BaseCollider;

public:
	OBBCollider();
	virtual ~OBBCollider();

	virtual bool Intersects(Ray& ray, float& distance) override;
	virtual bool Intersects(BaseCollider* other) override;

	BoundingOrientedBox& GetBoundingBox() { return _boundingBox; }

	Vec3 GetBoxExtents() const { return _boxExtents; }
	void SetBoxExtents(const Vec3& e) { _boxExtents = e; }

protected:
	virtual void UpdateBounds() override;

	// 디버그 시각화: 하늘색 Cube, 회전까지 반영
	virtual std::wstring GetDebugMeshKey() const override { return L"Cube"; }
	virtual Vec4         GetDebugColor()   const override { return Vec4(0.f, 0.5f, 1.f, 1.f); }
	virtual Matrix       GetDebugWorldMatrix() override;

private:
	Vec3				_boxExtents = { 0.5f, 0.5f, 0.5f };
	BoundingOrientedBox	_boundingBox = {};
};