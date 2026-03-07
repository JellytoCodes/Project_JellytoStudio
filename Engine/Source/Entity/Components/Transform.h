#pragma once

#include "Component.h"

class Transform : public Component
{
	using Super = Component;

public :
	Transform();
	virtual ~Transform();

	virtual void Awake() override;
	virtual void Start() override;
	virtual void Update() override;
	virtual void LateUpdate() override;
	virtual void OnDestroy() override;

	void UpdateTransform();
	static Vec3 ToEulerAngles(Quaternion q);

	// Local
	Vec3 GetLocalScale() const							{ return _localScale; }
	void SetLocalScale(const Vec3& localScale)			{ _localScale = localScale; UpdateTransform(); }

	Vec3 GetLocalRotation() const						{ return _localRotation; }
	void SetLocalRotation(const Vec3& localRotation)	{ _localRotation = localRotation; UpdateTransform(); }

	Vec3 GetLocalPosition() const						{ return _localPosition; }
	void SetLocalPosition(const Vec3& localPosition)	{ _localPosition = localPosition; UpdateTransform(); }

	// World
	Vec3 GetScale() const								{ return _scale; }
	void SetScale(const Vec3& scale);

	Vec3 GetRotation() const							{ return _rotation; }
	void SetRotation(const Vec3& rotation);

	Vec3 GetPosition() const							{ return _position; }
	void SetPosition(const Vec3& position);

	Vec3 GetRight() const								{ return _matWorld.Right(); }
	Vec3 GetUp() const									{ return _matWorld.Up(); }
	Vec3 GetLook() const								{ return _matWorld.Backward(); }

	Matrix GetWorldMatrix() const						{ return _matWorld; }

	// °èÃþ °ü°è
	bool HasParent() const								{ return !_parent.expired(); }
	
	shared_ptr<Transform> GetParent() const				{ return _parent.lock(); }
	void SetParent(const shared_ptr<Transform>& parent)	{ _parent = parent; }

	const vector<shared_ptr<Transform>>& GetChildren()	{ return _children; }
	void AddChild(const shared_ptr<Transform>& child)	{ _children.push_back(child); }

private:
	Vec3 _localScale		= { 1.f, 1.f, 1.f }; 
	Vec3 _localRotation		= { 0.f, 0.f, 0.f };
	Vec3 _localPosition		= { 0.f, 0.f, 0.f };

	// Cache
	Matrix _matLocal		= Matrix::Identity;
	Matrix _matWorld		= Matrix::Identity;
	
	Vec3 _scale;
	Vec3 _rotation;
	Vec3 _position;

	weak_ptr<Transform> _parent;
	vector<shared_ptr<Transform>> _children;
};

