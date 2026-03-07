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

	void CreateQuad();
	void CreateCube();
	void CreateGrid(int32 sizeX, int32 sizeZ);
	void CreateSphere();

	//shared_ptr<Geometry<VertexTextureNormalTangentData>> GetGeometry() { return _geometry; }
	shared_ptr<VertexBuffer> GetVertexBuffer() { return _vertexBuffer; }
	shared_ptr<IndexBuffer> GetIndexBuffer() { return _indexBuffer; }

private:
	void CreateBuffers();

private:
	// Mesh
	//shared_ptr<Geometry<VertexTextureNormalTangentData>> _geometry;
	shared_ptr<VertexBuffer> _vertexBuffer;
	shared_ptr<IndexBuffer> _indexBuffer;

};

