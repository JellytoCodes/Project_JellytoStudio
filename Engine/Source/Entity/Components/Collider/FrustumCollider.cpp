#include "Framework.h"
#include "AABBCollider.h"
#include "FrustumCollider.h"
#include "OBBCollider.h"
#include "SphereCollider.h"

FrustumCollider::FrustumCollider()
	: Super(ColliderType::Frustum)
{
}

FrustumCollider::~FrustumCollider()
{
}

void FrustumCollider::UpdateBounds()
{
	_boundingFrustum.Transform(_boundingFrustum, _colliderWorld);
}

bool FrustumCollider::Intersects(Ray& ray, float& distance)
{
	return _boundingFrustum.Intersects(ray.position, ray.direction, OUT distance);
}

bool FrustumCollider::Intersects(std::shared_ptr<BaseCollider>& other)
{
	switch (other->GetColliderType())
	{
	case ColliderType::Sphere:
		return _boundingFrustum.Intersects(dynamic_pointer_cast<SphereCollider>(other)->GetBoundingSphere());
	case ColliderType::AABB:
		return _boundingFrustum.Intersects(dynamic_pointer_cast<AABBCollider>(other)->GetBoundingBox());
	case ColliderType::OBB:
		return _boundingFrustum.Intersects(dynamic_pointer_cast<OBBCollider>(other)->GetBoundingBox());
	case ColliderType::Frustum:
		return _boundingFrustum.Intersects(dynamic_pointer_cast<FrustumCollider>(other)->GetBoundingFrustum());
	}
	return false;
}