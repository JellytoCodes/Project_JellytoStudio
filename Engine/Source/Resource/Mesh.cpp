#include "Framework.h"
#include "Mesh.h"

Mesh::Mesh() : Super(ResourceType::Mesh) {}
Mesh::~Mesh() {}

void Mesh::CreateQuad(const ComPtr<ID3D11Device>& device)
{
	_geometry = std::make_unique<Geometry<VertexTextureNormalTangentData>>();
	GeometryHelper::CreateQuad(_geometry.get());
	CreateBuffers(device);
}

void Mesh::CreateCube(const ComPtr<ID3D11Device>& device)
{
	_geometry = std::make_unique<Geometry<VertexTextureNormalTangentData>>();
	GeometryHelper::CreateCube(_geometry.get());
	CreateBuffers(device);
}

void Mesh::CreateGrid(const ComPtr<ID3D11Device>& device, int32 sizeX, int32 sizeZ)
{
	_geometry = std::make_unique<Geometry<VertexTextureNormalTangentData>>();
	GeometryHelper::CreateGrid(_geometry.get(), sizeX, sizeZ);
	CreateBuffers(device);
}

void Mesh::CreateSphere(const ComPtr<ID3D11Device>& device)
{
	_geometry = std::make_unique<Geometry<VertexTextureNormalTangentData>>();
	GeometryHelper::CreateSphere(_geometry.get());
	CreateBuffers(device);
}

void Mesh::Bind(const ComPtr<ID3D11DeviceContext>& deviceContext)
{
	_vertexBuffer->PushData(deviceContext);
	_indexBuffer->PushData(deviceContext);
	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void Mesh::CreateBuffers(const ComPtr<ID3D11Device>& device)
{
	// 이전: make_shared<VertexBuffer/IndexBuffer>() → 제어블록 별도 할당 ×2
	// 변경: make_unique → 단일 할당, Mesh가 유일한 소유자
	_vertexBuffer = std::make_unique<VertexBuffer>();
	_vertexBuffer->Create(device, _geometry->GetVertices());

	_indexBuffer = std::make_unique<IndexBuffer>();
	_indexBuffer->Create(device, _geometry->GetIndices());
}
