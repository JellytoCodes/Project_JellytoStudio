
#include "Framework.h"
#include "Entity/Components/MeshRenderer.h"
#include "Entity/Components/Camera.h"
#include "Entity/Components/Transform.h"
#include "Resource/Material.h"
#include "Resource/Mesh.h"
#include "Graphics/Graphics.h"

#include "Pipeline/Shader.h"

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
	auto device = Graphics::Get()->GetDevice();

	_mesh = std::make_shared<Mesh>();
	_mesh->CreateCube(device);

	_shader = std::make_shared<Shader>();
	_shader->Create(device, L"../Engine/Shaders/Default.hlsl");

	_constantBuffer = std::make_shared<ConstantBuffer<TransformData>>();
	_constantBuffer->Create(device);

	_matWorld = Matrix::Identity;
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

void MeshRenderer::Render()
{
auto deviceContext = Graphics::Get()->GetDeviceContext();

    _mesh->Bind(deviceContext);
    _shader->Bind(deviceContext);

    TransformData data;
    data.world = _matWorld.Transpose();

    data.view = Camera::S_MatView.Transpose();
    data.projection = Camera::S_MatProjection.Transpose();

    _constantBuffer->CopyData(deviceContext, data);

    auto bufferPtr = _constantBuffer->GetComPtr();
    deviceContext->VSSetConstantBuffers(0, 1, bufferPtr.GetAddressOf());
    deviceContext->PSSetConstantBuffers(0, 1, bufferPtr.GetAddressOf());

    deviceContext->DrawIndexed(_mesh->GetIndexBuffer()->GetCount(), 0, 0);
}