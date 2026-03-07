
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
	static float rotation = 0.f;
	rotation += 0.001f; 

	_matWorld = Matrix::CreateRotationY(rotation) * Matrix::CreateRotationX(rotation);	
}

void MeshRenderer::LateUpdate()
{
	
}

void MeshRenderer::OnDestroy()
{
	
}

void MeshRenderer::Render()
{
	auto device = Graphics::Get()->GetDevice();
    auto deviceContext = Graphics::Get()->GetDeviceContext();

	_mesh->Bind(deviceContext);
    _shader->Bind(deviceContext);

	Vec3 eye(0.f, 0.f, 3.f);
	Vec3 focus(0.f, 0.f, 0.f);
	Vec3 up(0.f, 1.f, 0.f);
	Matrix matView = Matrix::CreateLookAt(eye, focus, up);

	float aspectRatio = 800.f / 600.f;
	Matrix matProj = Matrix::CreatePerspectiveFieldOfView(XM_PI / 4.f, aspectRatio, 0.1f, 1000.f);

	TransformData data;
	data.world = _matWorld.Transpose();
	data.view = matView.Transpose();
	data.projection = matProj.Transpose();

    _constantBuffer->CopyData(deviceContext, data);

	auto bufferPtr = _constantBuffer->GetComPtr();
	deviceContext->VSSetConstantBuffers(0, 1, bufferPtr.GetAddressOf());
	deviceContext->PSSetConstantBuffers(0, 1, bufferPtr.GetAddressOf());

	deviceContext->DrawIndexed(_mesh->GetIndexBuffer()->GetCount(), 0, 0);
}