
#include "Framework.h"
#include "ShaderPass.h"

void ShaderPass::Draw(UINT vertexCount, UINT startVertexLocation)
{

}

void ShaderPass::DrawIndexed(UINT indexCount, UINT startIndexLocation, INT baseVertexLocation)
{

}

void ShaderPass::DrawInstanced(UINT vertexCountPerInstance, UINT instanceCount, UINT startVertexLocation,
	UINT startInstanceLocation)
{

}

void ShaderPass::DrawIndexedInstanced(UINT indexCountPerInstance, UINT instanceCount, UINT startIndexLocation,
	INT baseVertexLocation, UINT startInstanceLocation)
{

}

void ShaderPass::BeginDraw()
{
	pass->ComputeStateBlockMask(&stateblockMask);

	->IASetInputLayout(inputLayout.Get());
	pass->Apply(0, DC.Get());
}

void ShaderPass::EndDraw()
{

}

void ShaderPass::Dispatch(UINT x, UINT y, UINT z)
{

}
