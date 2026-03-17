
#include "Framework.h"
#include "BaseCollider.h"
#include "Entity/Components/Transform.h"

BaseCollider::BaseCollider(ColliderType colliderType)
	: Super(ComponentType::Collider), _colliderType(colliderType)
{

}

BaseCollider::~BaseCollider()
{

}

void BaseCollider::Update()
{
	Matrix matScale = Matrix::CreateScale(_offsetScale);

	Matrix matRotation = Matrix::CreateRotationX(_offsetRotation.x) 
	* Matrix::CreateRotationY(_offsetRotation.y) 
	* Matrix::CreateRotationZ(_offsetRotation.z);

	Matrix matTranslation = Matrix::CreateTranslation(_offsetPosition);

	Matrix matOffset = matScale * matRotation * matTranslation;

	_colliderWorld = matOffset * GetTransform()->GetWorldMatrix();

	// 자식 클래스에서 BoundingXxx 업데이트
	UpdateBounds();
}
