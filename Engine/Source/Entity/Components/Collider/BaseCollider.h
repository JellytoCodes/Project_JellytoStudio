#pragma once
#include "Entity/Components/Component.h"

enum class ColliderType : uint8
{
	Sphere,
	AABB,
	OBB,
	Frustum,

	End
};

class BaseCollider : public Component
{
	using Super = Component;
public:
	BaseCollider(ColliderType colliderType);
	virtual ~BaseCollider();

	virtual void Update() override;

	virtual bool Intersects(Ray& ray, OUT float& distance) = 0;
	virtual bool Intersects(std::shared_ptr<BaseCollider>& other) = 0;

	ColliderType GetColliderType() const { return _colliderType; }

	Vec3 GetOffsetPosition() const { return _offsetPosition; }
	void SetOffsetPosition(const Vec3& pos) { _offsetPosition = pos; }

	Vec3 GetOffsetRotation() const { return _offsetRotation; }
	void SetOffsetRotation(const Vec3& rot) { _offsetRotation = rot; }

	Vec3 GetOffsetScale() const { return _offsetScale; }
	void SetOffsetScale(const Vec3& scale) { _offsetScale = scale; }

	Matrix GetColliderWorldMatrix() const { return _colliderWorld; }

protected:
	virtual void UpdateBounds() = 0;

	ColliderType _colliderType;

	Vec3   _offsetPosition = { 0.f, 0.f, 0.f };
	Vec3   _offsetRotation = { 0.f, 0.f, 0.f };
	Vec3   _offsetScale = { 1.f, 1.f, 1.f };

	Matrix _colliderWorld = Matrix::Identity;
};