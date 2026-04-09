#pragma once

#include "Pipeline/IndexBuffer.h"
#include "Pipeline/VertexBuffer.h"
#include "Resource.h"

class Mesh : public Resource
{
	using Super = Resource;

public:
	Mesh();
	virtual ~Mesh();

	void CreateQuad(const ComPtr<ID3D11Device>& device);
	void CreateCube(const ComPtr<ID3D11Device>& device);
	void CreateGrid(const ComPtr<ID3D11Device>& device, int32 sizeX, int32 sizeZ);
	void CreateSphere(const ComPtr<ID3D11Device>& device);

	void Bind(const ComPtr<ID3D11DeviceContext>& deviceContext);

	Geometry<VertexTextureNormalTangentData>* GetGeometry()    { return _geometry.get(); }
	VertexBuffer* GetVertexBuffer() { return _vertexBuffer.get(); }
	IndexBuffer*  GetIndexBuffer()  { return _indexBuffer.get(); }

private:
	void CreateBuffers(const ComPtr<ID3D11Device>& device);

	// Mesh가 유일한 소유자 → unique_ptr
	std::unique_ptr<Geometry<VertexTextureNormalTangentData>> _geometry;
	std::unique_ptr<VertexBuffer>                             _vertexBuffer;
	std::unique_ptr<IndexBuffer>                              _indexBuffer;
};
