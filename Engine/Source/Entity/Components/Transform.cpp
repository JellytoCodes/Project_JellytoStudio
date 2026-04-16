
#include "Framework.h"
#include "Entity/Components/Transform.h"

Transform::Transform() 
	: Super(ComponentType::Transform)
{
	
}

Transform::~Transform()
{
	
}

void Transform::Awake()
{
	
}

void Transform::Start()
{
	
}

void Transform::Update()
{
	
}

void Transform::LateUpdate()
{
	
}

void Transform::OnDestroy()
{
	
}

Vec3 Transform::ToEulerAngles(Quaternion q)
{
	Vec3 angles;
	float sinr_cosp = 2 * (q.w * q.x + q.y * q.z);
	float cosr_cosp = 1 - 2 * (q.x * q.x + q.y * q.y);
	angles.x = std::atan2(sinr_cosp, cosr_cosp);

	float sinp = std::sqrt(1 + 2 * (q.w * q.y - q.x * q.z));
	float cosp = std::sqrt(1 - 2 * (q.w * q.y - q.x * q.z));
	angles.y = 2 * std::atan2(sinp, cosp) - 3.14159f / 2;

	float siny_cosp = 2 * (q.w * q.z + q.x * q.y);
	float cosy_cosp = 1 - 2 * (q.y * q.y + q.z * q.z);
	angles.z = std::atan2(siny_cosp, cosy_cosp);
	return angles;
}

void Transform::UpdateTransform()
{
	Matrix matScale    = Matrix::CreateScale(_localScale);
	Matrix matRotation = Matrix::CreateRotationX(_localRotation.x)
		* Matrix::CreateRotationY(_localRotation.y)
		* Matrix::CreateRotationZ(_localRotation.z);
	Matrix matTranslation = Matrix::CreateTranslation(_localPosition);

	_matLocal = matScale * matRotation * matTranslation;

	if (_parent != nullptr)
		_matWorld = _matLocal * _parent->GetWorldMatrix();
	else
		_matWorld = _matLocal;

	Quaternion quat;
	_matWorld.Decompose(_scale, quat, _position);
	_rotation = ToEulerAngles(quat);

	for (Transform* child : _children)
		child->UpdateTransform();
}

void Transform::SetScale(const Vec3& worldScale)
{
	if (_parent != nullptr)
	{
		Vec3 parentScale = _parent->GetScale();
		SetLocalScale({ worldScale.x / parentScale.x,
		                worldScale.y / parentScale.y,
		                worldScale.z / parentScale.z });
	}
	else
		SetLocalScale(worldScale);
}

void Transform::SetRotation(const Vec3& worldRotation)
{
	if (_parent != nullptr)
	{
		Matrix inverseMatrix = _parent->GetWorldMatrix().Invert();
		Vec3 rotation;
		rotation.TransformNormal(worldRotation, inverseMatrix);
		SetLocalRotation(rotation);
	}
	else
		SetLocalRotation(worldRotation);
}

void Transform::SetPosition(const Vec3& worldPosition)
{
	if (_parent != nullptr)
	{
		Matrix worldToParentLocal = _parent->GetWorldMatrix().Invert();
		Vec3 position;
		position.Transform(worldPosition, worldToParentLocal);
		SetLocalPosition(position);
	}
	else
		SetLocalPosition(worldPosition);
}
