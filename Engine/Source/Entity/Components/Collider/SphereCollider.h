#pragma once
#include "BaseCollider.h"

class SphereCollider : public BaseCollider
{
	using Super = BaseCollider;

public:
	SphereCollider();
	virtual ~SphereCollider();

	virtual bool Intersects(Ray& ray, float& distance) override;
	virtual bool Intersects(BaseCollider* other) override;

	BoundingSphere& GetBoundingSphere()				{ return _boundingSphere; }

	float GetRadius() const							{ return _radius; }
	void SetRadius(float r)							{ _radius = r; }

protected:
	virtual void UpdateBounds() override;

	// 디버그 시각화
	virtual std::wstring GetDebugMeshKey() const override	{ return L"Sphere"; }
	virtual Vec4         GetDebugColor()   const override	{ return Vec4(1.f, 1.f, 0.f, 1.f); }
	virtual Matrix       GetDebugWorldMatrix() override;

private:
	float			_radius = 0.f;
	BoundingSphere	_boundingSphere = {};
};