
#include "Framework.h"
#include "ShaderPass.h"

#include "Graphics/Graphics.h"

void ShaderPass::Draw(UINT vertexCount, UINT startVertexLocation)
{
	BeginDraw();

	Graphics::Get()->GetDeviceContext()->Draw(vertexCount, startVertexLocation);

	EndDraw();
}

void ShaderPass::DrawIndexed(UINT indexCount, UINT startIndexLocation, INT baseVertexLocation)
{
	BeginDraw();

	Graphics::Get()->GetDeviceContext()->DrawIndexed(indexCount, startIndexLocation, baseVertexLocation);

	EndDraw();
}

void ShaderPass::DrawInstanced(UINT vertexCountPerInstance, UINT instanceCount, UINT startVertexLocation, UINT startInstanceLocation)
{
	BeginDraw();

	Graphics::Get()->GetDeviceContext()->DrawInstanced(vertexCountPerInstance, instanceCount, startVertexLocation, startInstanceLocation);

	EndDraw();
}

void ShaderPass::DrawIndexedInstanced(UINT indexCountPerInstance, UINT instanceCount, UINT startIndexLocation,
	INT baseVertexLocation, UINT startInstanceLocation)
{
	BeginDraw();

	Graphics::Get()->GetDeviceContext()->DrawIndexedInstanced(indexCountPerInstance, instanceCount, startIndexLocation, baseVertexLocation, startInstanceLocation);

	EndDraw();
}

void ShaderPass::BeginDraw()
{
	pass->ComputeStateBlockMask(&stateBlockMask);

	Graphics::Get()->GetDeviceContext()->IASetInputLayout(inputLayout.Get());
	pass->Apply(0, Graphics::Get()->GetDeviceContext().Get());
}

void ShaderPass::EndDraw()
{
	if (stateBlockMask.RSRasterizerState == 1)
		Graphics::Get()->GetDeviceContext()->RSSetState(stateBlock->RSRasterizerState.Get());

	if (stateBlockMask.OMDepthStencilState == 1)
		Graphics::Get()->GetDeviceContext()->OMSetDepthStencilState(stateBlock->OMDepthStencilState.Get(), stateBlock->OMStencilRef);

	if (stateBlockMask.OMBlendState == 1)
		Graphics::Get()->GetDeviceContext()->OMSetBlendState(stateBlock->OMBlendState.Get(), stateBlock->OMBlendFactor, stateBlock->OMSampleMask);

	Graphics::Get()->GetDeviceContext()->HSSetShader(nullptr, nullptr, 0);
	Graphics::Get()->GetDeviceContext()->DSSetShader(nullptr, nullptr, 0);
	Graphics::Get()->GetDeviceContext()->GSSetShader(nullptr, nullptr, 0);
}

void ShaderPass::Dispatch(UINT x, UINT y, UINT z)
{
	pass->Apply(0, Graphics::Get()->GetDeviceContext().Get());
	Graphics::Get()->GetDeviceContext()->Dispatch(x, y, z);

	ID3D11ShaderResourceView* null[1] = { 0 };
	Graphics::Get()->GetDeviceContext()->CSSetShaderResources(0, 1, null);

	ID3D11UnorderedAccessView* nullUav[1] = { 0 };
	Graphics::Get()->GetDeviceContext()->CSSetUnorderedAccessViews(0, 1, nullUav, nullptr);

	Graphics::Get()->GetDeviceContext()->CSSetShader(nullptr, nullptr, 0);
}
