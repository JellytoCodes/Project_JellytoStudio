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

	// ── 오프셋 ──────────────────────────────────────────────────────
	Vec3 GetOffsetPosition() const				{ return _offsetPosition; }
	void SetOffsetPosition(const Vec3& pos)		{ _offsetPosition = pos; }

	Vec3 GetOffsetRotation() const				{ return _offsetRotation; }
	void SetOffsetRotation(const Vec3& rot)		{ _offsetRotation = rot; }

	Vec3 GetOffsetScale() const					{ return _offsetScale; }
	void SetOffsetScale(const Vec3& scale)		{ _offsetScale = scale; }

	Matrix GetColliderWorldMatrix() const		{ return _colliderWorld; }

	// ── 콜리전 채널 ────────────────────────────────────────────
	// ownChannel  : 이 콜라이더가 속한 채널
	// pickableMask: 이 콜라이더를 피킹/감지할 수 있는 채널 비트마스크
	CollisionChannel GetOwnChannel()   const  { return _ownChannel; }
	void SetOwnChannel(CollisionChannel ch)   { _ownChannel = ch; }

	uint8 GetPickableMask()           const  { return _pickableMask; }
	void SetPickableMask(uint8 mask)         { _pickableMask = mask; }

	// queryChan 채널이 이 콜라이더를 피킹할 수 있는지
	bool CanBePickedBy(CollisionChannel queryChan) const
	{ return ChannelInMask(queryChan, _pickableMask); }

	// ── 정적 콜라이더 최적화 ──────────────────────────────────────────
	// SetStatic(true) → 최초 1회 계산 후 매프레임 행렬 연산 완전 스킵
	// 배치된 맵블록처럼 절대 안 움직이는 콜라이더에 사용
	bool IsStatic() const                   { return _isStatic; }
	void SetStatic(bool s)                  { _isStatic = s; _boundsReady = false; }
	void InvalidateBounds()                 { _boundsReady = false; } // 강제 재계산

	// ── 디버그 시각화 ────────────────────────────────────────────────
	bool IsShowDebug() const					{ return _showDebug; }
	void SetShowDebug(bool show)				{ _showDebug = show; }

	// Scene::Render()에서 호출 → 외곽선 렌더링
	void RenderDebug();

protected:
	virtual void UpdateBounds() = 0;

	// 자식이 구현: 어떤 메시(L"Cube"/L"Sphere")로 그릴지, 색상
	virtual std::wstring GetDebugMeshKey() const = 0;
	virtual Vec4         GetDebugColor()   const = 0;

	// 자식이 오버라이드: Extents/Radius를 반영한 실제 디버그 World 행렬
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

	bool _showDebug = true;

	// 모든 콜라이더가 공유하는 디버그 셰이더 (static)
	static std::shared_ptr<Shader> s_debugShader;
};