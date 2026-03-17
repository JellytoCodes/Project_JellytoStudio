
#include "Framework.h"
#include "AABBCollider.h"
#include "FrustumCollider.h"
#include "OBBCollider.h"
#include "SphereCollider.h"

AABBCollider::AABBCollider()
	: Super(ColliderType::AABB)
{
}

AABBCollider::~AABBCollider()
{
}

void AABBCollider::UpdateBounds()
{
	Vec3 scale, position;
	Quaternion quat;
	_colliderWorld.Decompose(scale, quat, position);

	_boundingBox.Center = position;
	_boundingBox.Extents = Vec3(
		_boxExtents.x * scale.x,
		_boxExtents.y * scale.y,
		_boxExtents.z * scale.z
	);
}

Matrix AABBCollider::GetDebugWorldMatrix()
{
	Vec3 scale, position;
	Quaternion quat;
	_colliderWorld.Decompose(scale, quat, position);

	// AABB는 회전 없음 → Identity Quat
	Vec3 debugScale(
		_boxExtents.x * scale.x * 2.f,
		_boxExtents.y * scale.y * 2.f,
		_boxExtents.z * scale.z * 2.f
	);

	return Matrix::CreateScale(debugScale) * Matrix::CreateTranslation(position);
}

bool AABBCollider::Intersects(Ray& ray, float& distance)
{
	return _boundingBox.Intersects(ray.position, ray.direction, OUT distance);
}

bool AABBCollider::Intersects(std::shared_ptr<BaseCollider>& other)
{
	switch (other->GetColliderType())
	{
	case ColliderType::Sphere:
		return _boundingBox.Intersects(dynamic_pointer_cast<SphereCollider>(other)->GetBoundingSphere());
	case ColliderType::AABB:
		return _boundingBox.Intersects(dynamic_pointer_cast<AABBCollider>(other)->GetBoundingBox());
	case ColliderType::OBB:
		return _boundingBox.Intersects(dynamic_pointer_cast<OBBCollider>(other)->GetBoundingBox());
	case ColliderType::Frustum:
		return _boundingBox.Intersects(dynamic_pointer_cast<FrustumCollider>(other)->GetBoundingFrustum());
	}
	return false;
}