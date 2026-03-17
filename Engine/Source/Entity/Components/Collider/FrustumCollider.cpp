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
	_localFrustum.Transform(_boundingFrustum, _colliderWorld);
}

Matrix FrustumCollider::GetDebugWorldMatrix()
{
	XMFLOAT3 corners[BoundingFrustum::CORNER_COUNT];
	_boundingFrustum.GetCorners(corners);

	Vec3 minPt = Vec3(corners[0].x, corners[0].y, corners[0].z);
	Vec3 maxPt = minPt;

	for (int i = 1; i < BoundingFrustum::CORNER_COUNT; i++)
	{
		minPt.x = std::min(minPt.x, corners[i].x);
		minPt.y = std::min(minPt.y, corners[i].y);
		minPt.z = std::min(minPt.z, corners[i].z);

		maxPt.x = max(maxPt.x, corners[i].x);
		maxPt.y = max(maxPt.y, corners[i].y);
		maxPt.z = max(maxPt.z, corners[i].z);
	}

	Vec3 center = (minPt + maxPt) * 0.5f;
	Vec3 size = maxPt - minPt;
	
	return Matrix::CreateScale(size)
		* Matrix::CreateTranslation(center);
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