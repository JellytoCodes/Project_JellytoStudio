#pragma once

#include "Utils/Geometry.h"
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

	//shared_ptr<Geometry<VertexTextureNormalTangentData>> GetGeometry() { return _geometry; }
	shared_ptr<VertexBuffer> GetVertexBuffer() { return _vertexBuffer; }
	shared_ptr<IndexBuffer> GetIndexBuffer() { return _indexBuffer; }

private:
	void CreateBuffers(const ComPtr<ID3D11Device>& device);

private:
	
	std::shared_ptr<Geometry<VertexColorData>> _geometry;
	std::shared_ptr<VertexBuffer> _vertexBuffer;
	std::shared_ptr<IndexBuffer> _indexBuffer;
};

