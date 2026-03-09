
#include "Framework.h"
#include "Mesh.h"

Mesh::Mesh()
	: Super(ResourceType::Mesh)
{

}

Mesh::~Mesh()
{

}

void Mesh::CreateQuad(const ComPtr<ID3D11Device>& device)
{
	_geometry = std::make_shared<Geometry<VertexColorData>>();
	GeometryHelper::CreateQuad(_geometry, Color(1.f, 1.f, 1.f, 1.f));
	CreateBuffers(device);
}

void Mesh::CreateCube(const ComPtr<ID3D11Device>& device)
{
	_geometry = std::make_shared<Geometry<VertexColorData>>();
	GeometryHelper::CreateCube(_geometry);
	CreateBuffers(device);
}

void Mesh::CreateGrid(const ComPtr<ID3D11Device>& device, int32 sizeX, int32 sizeZ)
{

}

void Mesh::CreateSphere(const ComPtr<ID3D11Device>& device)
{

}

void Mesh::Bind(const ComPtr<ID3D11DeviceContext>& deviceContext)
{
	_vertexBuffer->PushData(deviceContext);
    _indexBuffer->PushData(deviceContext);
    deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void Mesh::CreateBuffers(const ComPtr<ID3D11Device>& device)
{
	_vertexBuffer = std::make_shared<VertexBuffer>();
	_vertexBuffer->Create(device, _geometry->GetVertices());

	_indexBuffer = std::make_shared<IndexBuffer>();
	_indexBuffer->Create(device, _geometry->GetIndices());
}
