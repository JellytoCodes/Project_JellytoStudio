#pragma once
#include "Entity/Components/Component.h"
#include "Entity/Components/Collider/CollisionChannel.h"

enum class ColliderType : uint8
{
	Sphere,
	AABB,
	OBB,
	Frustum,

	End
};

class Shader;
class Mesh;

class BaseCollider : public Component
{
	using Super = Component;
public:
	BaseCollider(ColliderType colliderType);
	virtual ~BaseCollider();

	virtual void Update() override;

	virtual bool Intersects(Ray& ray, OUT float& distance) = 0;
	virtual bool Intersects(BaseCollider* other) = 0;

	ColliderType GetColliderType() const { return _colliderType; }

	Vec3 GetOffsetPosition() const				{ return _offsetPosition; }
	void SetOffsetPosition(const Vec3& pos)		{ _offsetPosition = pos; }

	Vec3 GetOffsetRotation() const				{ return _offsetRotation; }
	void SetOffsetRotation(const Vec3& rot)		{ _offsetRotation = rot; }

	Vec3 GetOffsetScale() const					{ return _offsetScale; }
	void SetOffsetScale(const Vec3& scale)		{ _offsetScale = scale; }

	Matrix GetColliderWorldMatrix() const		{ return _colliderWorld; }

	CollisionChannel GetOwnChannel()   const  { return _ownChannel; }
	void SetOwnChannel(CollisionChannel ch)   { _ownChannel = ch; }

	uint8 GetPickableMask()           const  { return _pickableMask; }
	void SetPickableMask(uint8 mask)         { _pickableMask = mask; }

	bool CanBePickedBy(CollisionChannel queryChan) const
	{ return ChannelInMask(queryChan, _pickableMask); }

	bool IsStatic() const                   { return _isStatic; }
	void SetStatic(bool s)                  { _isStatic = s; _boundsReady = false; }
	void InvalidateBounds()                 { _boundsReady = false; }

	bool IsShowDebug() const					{ return _showDebug; }
	void SetShowDebug(bool show)				{ _showDebug = show; }

	void RenderDebug();

protected:
	virtual void UpdateBounds() = 0;

	virtual std::wstring GetDebugMeshKey() const = 0;
	virtual Vec4         GetDebugColor()   const = 0;

	virtual Matrix GetDebugWorldMatrix()		{ return _colliderWorld; }

	ColliderType      _colliderType;

	CollisionChannel  _ownChannel    = CollisionChannel::Default;
	uint8            _pickableMask  = static_cast<uint8>(CollisionChannel::All);

	Vec3   _offsetPosition =	{ 0.f, 0.f, 0.f };
	Vec3   _offsetRotation =	{ 0.f, 0.f, 0.f };
	Vec3   _offsetScale =		{ 1.f, 1.f, 1.f };

	Matrix _colliderWorld = Matrix::Identity;

	bool   _isStatic    = false; // 정적 콜라이더 플래그
	bool   _boundsReady = false; // 최초 계산 완료 여부

private:
	void InitDebugShader();

	bool _showDebug = false;

	static std::shared_ptr<Shader> s_debugShader;
};