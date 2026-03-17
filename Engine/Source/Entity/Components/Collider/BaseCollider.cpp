#include "Framework.h"
#include "BaseCollider.h"
#include "Entity/Components/Camera.h"
#include "Entity/Components/Transform.h"
#include "Pipeline/Shader.h"
#include "Resource/Mesh.h"
#include "Resource/Managers/ResourceManager.h"
#include "Graphics/Graphics.h"

// static 멤버 정의
std::shared_ptr<Shader> BaseCollider::s_debugShader = nullptr;

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

	UpdateBounds();
}

void BaseCollider::InitDebugShader()
{
	if (s_debugShader != nullptr) return;
	s_debugShader = std::make_shared<Shader>(L"../Engine/Shaders/ColliderDebug.hlsl");
}

void BaseCollider::RenderDebug()
{
	if (!_showDebug) return;

	InitDebugShader();

	auto mesh = GET_SINGLE(ResourceManager)->Get<Mesh>(GetDebugMeshKey());
	if (mesh == nullptr) return;

	// View/Projection 세팅
	s_debugShader->PushGlobalData(Camera::S_MatView, Camera::S_MatProjection);

	// 콜라이더 크기(Extents/Radius)가 반영된 World 행렬 전달
	s_debugShader->PushTransformData(TransformDesc{ GetDebugWorldMatrix() });

	// 색상 전달
	Vec4 color = GetDebugColor();
	s_debugShader->GetVector("ColliderColor")->SetFloatVector(reinterpret_cast<float*>(&color));

	// IA 세팅 후 Draw (인스턴싱 없이 단순 DrawIndexed)
	auto dc = Graphics::Get()->GetDeviceContext();
	mesh->GetVertexBuffer()->PushData(dc);
	mesh->GetIndexBuffer()->PushData(dc);
	dc->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	s_debugShader->DrawIndexed(0, 0, mesh->GetIndexBuffer()->GetCount());
}