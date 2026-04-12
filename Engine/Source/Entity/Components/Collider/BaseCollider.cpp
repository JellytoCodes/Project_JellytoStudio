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
	// ── 정적 콜라이더 조기 탈출 ────────────────────────────────────
	// _isStatic=true & 이미 계산됨 → 행렬 연산/분해 전부 스킵
	// 효과: 블록 N개 × (행렬 4회 곱 + XMMatrixDecompose) 절감/프레임
	if (_isStatic && _boundsReady) return;

	Matrix matScale = Matrix::CreateScale(_offsetScale);
	Matrix matRotation = Matrix::CreateRotationX(_offsetRotation.x)
		* Matrix::CreateRotationY(_offsetRotation.y)
		* Matrix::CreateRotationZ(_offsetRotation.z);
	Matrix matTranslation = Matrix::CreateTranslation(_offsetPosition);
	Matrix matOffset = matScale * matRotation * matTranslation;

	_colliderWorld = matOffset * GetTransform()->GetWorldMatrix();

	UpdateBounds();

	// 정적 콜라이더: 최초 계산 완료 표시 → 이후 Update() 즉시 return
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

	// View/Projection 세팅
	s_debugShader->PushGlobalData(Camera::S_MatView, Camera::S_MatProjection);

	// 콜라이더 크기(Extents/Radius)가 반영된 World 행렬 전달
	s_debugShader->PushTransformData(TransformDesc{ GetDebugWorldMatrix() });

	// 색상 전달
	Vec4 color = GetDebugColor();
	s_debugShader->GetVector("ColliderColor")->SetFloatVector(reinterpret_cast<float*>(&color));

	// IA 세팅 후 Draw (인스턴싱 없이 단순 DrawIndexed)
	auto dc = Graphics::GET_SINGLE(Graphics)->GetDeviceContext();
	mesh->GetVertexBuffer()->PushData(dc);
	mesh->GetIndexBuffer()->PushData(dc);
	dc->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	s_debugShader->DrawIndexed(0, 0, mesh->GetIndexBuffer()->GetCount());
}