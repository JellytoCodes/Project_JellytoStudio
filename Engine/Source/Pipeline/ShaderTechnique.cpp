
#include "Framework.h"
#include "ShaderTechnique.h"
#include "ShaderPass.h"

void Technique::Draw(UINT pass, UINT vertexCount, UINT startVertexLocation)
{
	if (pass >= passes.size()) return;
	passes[pass].Draw(vertexCount, startVertexLocation);
}

void Technique::DrawIndexed(UINT pass, UINT indexCount, UINT startIndexLocation, INT baseVertexLocation)
{
	if (pass >= passes.size()) return;
	passes[pass].DrawIndexed(indexCount, startIndexLocation, baseVertexLocation);
}

void Technique::DrawInstanced(UINT pass, UINT vertexCountPerInstance, UINT instanceCount, UINT startVertexLocation,
	UINT startInstanceLocation)
{
	if (pass >= passes.size()) return;
	passes[pass].DrawInstanced(vertexCountPerInstance, instanceCount, startVertexLocation, startInstanceLocation);
}

void Technique::DrawIndexedInstanced(UINT pass, UINT indexCountPerInstance, UINT instanceCount, UINT startIndexLocation,
	INT baseVertexLocation, UINT startInstanceLocation)
{
	if (pass >= passes.size()) return;
	passes[pass].DrawIndexedInstanced(indexCountPerInstance, instanceCount, startIndexLocation, baseVertexLocation, startInstanceLocation);
}

void Technique::Dispatch(UINT pass, UINT x, UINT y, UINT z)
{
	if (pass >= passes.size()) return;
	passes[pass].Dispatch(x, y, z);
}
