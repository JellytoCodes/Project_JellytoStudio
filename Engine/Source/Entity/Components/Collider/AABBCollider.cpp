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

bool AABBCollider::Intersects(BaseCollider* other)
{
	switch (other->GetColliderType())
	{
	case ColliderType::Sphere:
		return _boundingBox.Intersects(static_cast<SphereCollider*>(other)->GetBoundingSphere());
	case ColliderType::AABB:
		return _boundingBox.Intersects(static_cast<AABBCollider*>(other)->GetBoundingBox());
	case ColliderType::OBB:
		return _boundingBox.Intersects(static_cast<OBBCollider*>(other)->GetBoundingBox());
	case ColliderType::Frustum:
		return _boundingBox.Intersects(static_cast<FrustumCollider*>(other)->GetBoundingFrustum());
	}
	return false;
}
bool AABBCollider::IntersectsWithNormal(Ray& ray, float& distance, Vec3& outHitNormal)
{
	if (!_boundingBox.Intersects(ray.position, ray.direction, distance))
		return false;

	// 히트 포인트
	Vec3 hitPoint = ray.position + ray.direction * distance;

	// 박스 중심 기준 상대 좌표를 extents로 나눔 → 가장 큰 성분 = 히트 면
	Vec3 c = _boundingBox.Center;
	Vec3 e = _boundingBox.Extents;
	Vec3 d(
		(hitPoint.x - c.x) / (e.x > 1e-6f ? e.x : 1e-6f),
		(hitPoint.y - c.y) / (e.y > 1e-6f ? e.y : 1e-6f),
		(hitPoint.z - c.z) / (e.z > 1e-6f ? e.z : 1e-6f)
	);

	float ax = fabsf(d.x), ay = fabsf(d.y), az = fabsf(d.z);
	if (ax >= ay && ax >= az)
		outHitNormal = Vec3(d.x > 0.f ? 1.f : -1.f, 0.f, 0.f);
	else if (ay >= az)
		outHitNormal = Vec3(0.f, d.y > 0.f ? 1.f : -1.f, 0.f);
	else
		outHitNormal = Vec3(0.f, 0.f, d.z > 0.f ? 1.f : -1.f);

	return true;
}