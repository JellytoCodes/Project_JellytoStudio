#include "Framework.h"
#include "BaseCollider.h"
#include "Entity/Components/Camera.h"
#include "Entity/Components/Transform.h"
#include "Pipeline/Shader.h"
#include "Resource/Mesh.h"
#include "Resource/Managers/ResourceManager.h"
#include "Graphics/Graphics.h"

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
	if (_isStatic && _boundsReady) return;

	Matrix matScale = Matrix::CreateScale(_offsetScale);
	Matrix matRotation = Matrix::CreateRotationX(_offsetRotation.x)
		* Matrix::CreateRotationY(_offsetRotation.y)
		* Matrix::CreateRotationZ(_offsetRotation.z);
	Matrix matTranslation = Matrix::CreateTranslation(_offsetPosition);
	Matrix matOffset = matScale * matRotation * matTranslation;

	_colliderWorld = matOffset * GetTransform()->GetWorldMatrix();

	UpdateBounds();

	if (_isStatic) _boundsReady = true;
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

	s_debugShader->PushGlobalData(Camera::S_MatView, Camera::S_MatProjection);
	s_debugShader->PushTransformData(TransformDesc{ GetDebugWorldMatrix() });

	Vec4 color = GetDebugColor();
	s_debugShader->GetVector("ColliderColor")->SetFloatVector(reinterpret_cast<float*>(&color));

	auto dc = Graphics::GET_SINGLE(Graphics)->GetDeviceContext();
	mesh->GetVertexBuffer()->PushData(dc);
	mesh->GetIndexBuffer()->PushData(dc);
	dc->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	s_debugShader->DrawIndexed(0, 0, mesh->GetIndexBuffer()->GetCount());
}