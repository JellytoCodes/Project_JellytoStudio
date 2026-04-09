#pragma once

#include "Geometry.h"

class GeometryHelper
{
public :
	static void CreateQuad(Geometry<VertexColorData>* geometry, Color color);
	static void CreateCube(Geometry<VertexColorData>* geometry);

	static void CreateQuad(Geometry<VertexTextureData>* geometry);
	static void CreateCube(Geometry<VertexTextureData>* geometry);
	static void CreateSphere(Geometry<VertexTextureData>* geometry, uint32 stackCount, uint32 sliceCount);
	static void CreateGrid(Geometry<VertexTextureData>* geometry, int32 sizeX, int32 sizeZ);

	static void CreateQuad(Geometry<VertexTextureNormalData>* geometry);
	static void CreateCube(Geometry<VertexTextureNormalData>* geometry);
	static void CreateGrid(Geometry<VertexTextureNormalData>* geometry, int32 sizeX, int32 sizeZ);
	static void CreateSphere(Geometry<VertexTextureNormalData>* geometry);

	static void CreateQuad(Geometry<VertexTextureNormalTangentData>* geometry);
	static void CreateCube(Geometry<VertexTextureNormalTangentData>* geometry);
	static void CreateGrid(Geometry<VertexTextureNormalTangentData>* geometry, int32 sizeX, int32 sizeZ);
	static void CreateSphere(Geometry<VertexTextureNormalTangentData>* geometry);
};