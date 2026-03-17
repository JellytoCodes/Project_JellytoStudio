#pragma once
#include "BaseCollider.h"

class AABBCollider : public BaseCollider
{
	using Super = BaseCollider;

public:
	AABBCollider();
	virtual ~AABBCollider();

	virtual bool Intersects(Ray& ray, float& distance) override;
	virtual bool Intersects(std::shared_ptr<BaseCollider>& other) override;

	BoundingBox& GetBoundingBox()					{ return _boundingBox; }

	Vec3 GetBoxExtents() const						{ return _boxExtents; }
	void SetBoxExtents(const Vec3& e)				{ _boxExtents = e; }

protected:
	virtual void UpdateBounds() override;

	virtual std::wstring GetDebugMeshKey() const override	{ return L"Cube"; }
	virtual Vec4         GetDebugColor()   const override	{ return Vec4(1.f, 0.f, 0.f, 1.f); }
	virtual Matrix       GetDebugWorldMatrix() override;

private:
	Vec3		_boxExtents = { 0.f, 0.f, 0.f };
	BoundingBox	_boundingBox = {};
};