
#include "Framework.h"
#include "AABBCollider.h"
#include "FrustumCollider.h"
#include "OBBCollider.h"
#include "SphereCollider.h"

SphereCollider::SphereCollider()
	: Super(ColliderType::Sphere)
{

}

SphereCollider::~SphereCollider() { }

void SphereCollider::UpdateBounds()
{
	Vec3 scale, position;
	Quaternion quat;
	_colliderWorld.Decompose(scale, quat, position);

	float maxScale = scale.x;
	if (scale.y > maxScale) maxScale = scale.y;
	if (scale.z > maxScale) maxScale = scale.z;

	_boundingSphere.Center = position;
	_boundingSphere.Radius = _radius * maxScale;
}

Matrix SphereCollider::GetDebugWorldMatrix()
{
	Vec3 scale, position;
	Quaternion quat;
	_colliderWorld.Decompose(scale, quat, position);

	float maxScale = scale.x;
	if (scale.y > maxScale) maxScale = scale.y;
	if (scale.z > maxScale) maxScale = scale.z;

	float debugDiameter = _radius * maxScale * 2.f;

	return Matrix::CreateScale(Vec3(debugDiameter)) * Matrix::CreateTranslation(position);
}

bool SphereCollider::Intersects(Ray& ray, float& distance)
{
	return _boundingSphere.Intersects(ray.position, ray.direction, OUT distance);
}

bool SphereCollider::Intersects(BaseCollider* other)
{
	switch (other->GetColliderType())
	{
	case ColliderType::Sphere:
		return _boundingSphere.Intersects(static_cast<SphereCollider*>(other)->GetBoundingSphere());
	case ColliderType::AABB:
		return _boundingSphere.Intersects(static_cast<AABBCollider*>(other)->GetBoundingBox());
	case ColliderType::OBB:
		return _boundingSphere.Intersects(static_cast<OBBCollider*>(other)->GetBoundingBox());
	case ColliderType::Frustum:
		return _boundingSphere.Intersects(static_cast<FrustumCollider*>(other)->GetBoundingFrustum());
	}
	return false;
}