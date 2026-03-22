#include "Framework.h"
#include "Entity/Components/MeshRenderer.h"
#include "Entity/Components/Camera.h"
#include "Entity/Components/Transform.h"
#include "Entity/Components/Light.h"
#include "Resource/Material.h"
#include "Resource/Mesh.h"
#include "Graphics/Graphics.h"

#include "Pipeline/Shader.h"
#include "Scene/SceneManager.h"

MeshRenderer::MeshRenderer()
	: Super(ComponentType::MeshRenderer)
{

}

MeshRenderer::~MeshRenderer()
{

}

void MeshRenderer::Awake()
{

}

void MeshRenderer::Start()
{
	
}

void MeshRenderer::Update()
{

}

void MeshRenderer::LateUpdate()
{
	
}

void MeshRenderer::OnDestroy()
{
	
}

void MeshRenderer::RenderInstancing(const std::shared_ptr<InstancingBuffer>& buffer)
{
	if (_mesh == nullptr || _material == nullptr) return;

	std::shared_ptr<Shader> shader = _material->GetShader();
	if (shader == nullptr) return;

	// GlobalData
	shader->PushGlobalData(Camera::S_MatView, Camera::S_MatProjection);

	// Light
	if (std::shared_ptr<Light> lightObj = GET_SINGLE(SceneManager)->GetCurrentScene()->GetLight())
	{
		assert(lightObj.get() != nullptr && "[MeshRenderer] lightObj is null");
		shader->PushLightData(lightObj->GetLightDesc());
	}

	// Light
	_material->Update();

	// IA
	_mesh->GetVertexBuffer()->PushData(Graphics::Get()->GetDeviceContext());
	_mesh->GetIndexBuffer()->PushData(Graphics::Get()->GetDeviceContext());

	buffer->PushData();

	shader->DrawIndexedInstanced(0, _pass, _mesh->GetIndexBuffer()->GetCount(), buffer->GetCount());
}

InstanceID MeshRenderer::GetInstanceID()
{
	return std::make_pair((uint64)_mesh.get(), (uint64)_material.get());
}