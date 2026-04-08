#pragma once
#include "BaseCollider.h"

class FrustumCollider : public BaseCollider
{
	using Super = BaseCollider;

public:
	FrustumCollider();
	virtual ~FrustumCollider();

	virtual bool Intersects(Ray& ray, float& distance) override;
	virtual bool Intersects(BaseCollider* other) override;

	BoundingFrustum& GetBoundingFrustum() { return _boundingFrustum; }

	void SetFromProjection(const Matrix& matProjection)
	{
		BoundingFrustum::CreateFromMatrix(_boundingFrustum, matProjection);
		_localFrustum = _boundingFrustum;
	}

protected:
	virtual void UpdateBounds() override;

	virtual std::wstring GetDebugMeshKey() const override { return L"Cube"; }
	virtual Vec4         GetDebugColor()   const override { return Vec4(1.f, 0.5f, 0.f, 1.f); } // 주황
	virtual Matrix       GetDebugWorldMatrix() override;

private:
	BoundingFrustum _boundingFrustum = {};
	BoundingFrustum _localFrustum = {};
};