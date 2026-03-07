#pragma once

#include "Geometry.h"

class GeometryHelper
{
public :
	static void CreateQuad(const shared_ptr<Geometry<VertexColorData>>& geometry, Color color);
	static void CreateCube(const shared_ptr<Geometry<VertexColorData>>& geometry);

	static void CreateQuad(const shared_ptr<Geometry<VertexTextureData>>& geometry);
	static void CreateCube(const shared_ptr<Geometry<VertexTextureData>>& geometry);
	static void CreateSphere(const shared_ptr<Geometry<VertexTextureData>>& geometry, uint32 stackCount, uint32 sliceCount);
	static void CreateGrid(const shared_ptr<Geometry<VertexTextureData>>& geometry, int32 sizeX, int32 sizeZ);

	static void CreateQuad(const shared_ptr<Geometry<VertexTextureNormalData>>& geometry);
	static void CreateCube(const shared_ptr<Geometry<VertexTextureNormalData>>& geometry);
	static void CreateGrid(const shared_ptr<Geometry<VertexTextureNormalData>>& geometry, int32 sizeX, int32 sizeZ);
	static void CreateSphere(const shared_ptr<Geometry<VertexTextureNormalData>>& geometry);

	static void CreateQuad(const shared_ptr<Geometry<VertexTextureNormalTangentData>>& geometry);
	static void CreateCube(const shared_ptr<Geometry<VertexTextureNormalTangentData>>& geometry);
	static void CreateGrid(const shared_ptr<Geometry<VertexTextureNormalTangentData>>& geometry, int32 sizeX, int32 sizeZ);
	static void CreateSphere(const shared_ptr<Geometry<VertexTextureNormalTangentData>>& geometry);
};