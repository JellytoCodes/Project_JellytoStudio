
#include "Framework.h"
#include "ShaderPass.h"

#include "Graphics/Graphics.h"

void ShaderPass::Draw(UINT vertexCount, UINT startVertexLocation)
{
	if (pass == nullptr || vertexCount == 0) return;

	BeginDraw();

	GET_SINGLE(Graphics)->GetDeviceContext()->Draw(vertexCount, startVertexLocation);

	EndDraw();
}

void ShaderPass::DrawIndexed(UINT indexCount, UINT startIndexLocation, INT baseVertexLocation)
{
	if (pass == nullptr || indexCount == 0) return;

	BeginDraw();

	GET_SINGLE(Graphics)->GetDeviceContext()->DrawIndexed(indexCount, startIndexLocation, baseVertexLocation);

	EndDraw();
}

void ShaderPass::DrawInstanced(UINT vertexCountPerInstance, UINT instanceCount, UINT startVertexLocation, UINT startInstanceLocation)
{
	if (pass == nullptr || vertexCountPerInstance == 0 || instanceCount == 0) return;

	BeginDraw();

	GET_SINGLE(Graphics)->GetDeviceContext()->DrawInstanced(vertexCountPerInstance, instanceCount, startVertexLocation, startInstanceLocation);

	EndDraw();
}

void ShaderPass::DrawIndexedInstanced(UINT indexCountPerInstance, UINT instanceCount, UINT startIndexLocation,
	INT baseVertexLocation, UINT startInstanceLocation)
{
	if (pass == nullptr || indexCountPerInstance == 0 || instanceCount == 0) return;

	BeginDraw();

	GET_SINGLE(Graphics)->GetDeviceContext()->DrawIndexedInstanced(indexCountPerInstance, instanceCount, startIndexLocation, baseVertexLocation, startInstanceLocation);

	EndDraw();
}

void ShaderPass::BeginDraw()
{
	if (pass == nullptr) return;

	pass->ComputeStateBlockMask(&stateBlockMask);

	GET_SINGLE(Graphics)->GetDeviceContext()->IASetInputLayout(inputLayout.Get());
	pass->Apply(0, GET_SINGLE(Graphics)->GetDeviceContext().Get());
}

void ShaderPass::EndDraw()
{
	if (stateBlock == nullptr) return;

	if (stateBlockMask.RSRasterizerState == 1)
		GET_SINGLE(Graphics)->SetRasterizerState(stateBlock->RSRasterizerState.Get());

	if (stateBlockMask.OMDepthStencilState == 1)
		GET_SINGLE(Graphics)->SetDepthStencilState(stateBlock->OMDepthStencilState.Get(), stateBlock->OMStencilRef);

	if (stateBlockMask.OMBlendState == 1)
		GET_SINGLE(Graphics)->SetBlendState(stateBlock->OMBlendState.Get(), stateBlock->OMBlendFactor, stateBlock->OMSampleMask);

	GET_SINGLE(Graphics)->GetDeviceContext()->HSSetShader(nullptr, nullptr, 0);
	GET_SINGLE(Graphics)->GetDeviceContext()->DSSetShader(nullptr, nullptr, 0);
	GET_SINGLE(Graphics)->GetDeviceContext()->GSSetShader(nullptr, nullptr, 0);
}

void ShaderPass::Dispatch(UINT x, UINT y, UINT z)
{
	if (pass == nullptr || x == 0 || y == 0 || z == 0) return;

	pass->Apply(0, GET_SINGLE(Graphics)->GetDeviceContext().Get());
	GET_SINGLE(Graphics)->GetDeviceContext()->Dispatch(x, y, z);

	ID3D11ShaderResourceView* null[1] = { 0 };
	GET_SINGLE(Graphics)->GetDeviceContext()->CSSetShaderResources(0, 1, null);

	ID3D11UnorderedAccessView* nullUav[1] = { 0 };
	GET_SINGLE(Graphics)->GetDeviceContext()->CSSetUnorderedAccessViews(0, 1, nullUav, nullptr);

	GET_SINGLE(Graphics)->GetDeviceContext()->CSSetShader(nullptr, nullptr, 0);
}
