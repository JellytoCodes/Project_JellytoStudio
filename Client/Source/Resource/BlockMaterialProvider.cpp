#include "pch.h"
#include "BlockMaterialProvider.h"

#include "Data/BlockTable.h"
#include "Resource/Managers/ResourceManager.h"
#include "Resource/Material.h"
#include "Pipeline/Shader.h"
#include "Graphics/Graphics.h"

void BlockMaterialProvider::Init()
{
	auto* table = GET_SINGLE(BlockTable);

	auto paletteTex = GET_SINGLE(ResourceManager)->GetOrAddTexture(
		L"BlockPalette", table->GetPalettePath());

	_blockShader = std::make_shared<Shader>(L"../Engine/Shaders/BlockShader.hlsl");

	_blockMaterial = std::make_shared<Material>();
	_blockMaterial->SetShader(_blockShader);
	_blockMaterial->SetDiffuseMap(paletteTex);
	auto& desc    = _blockMaterial->GetMaterialDesc();
	desc.ambient  = Vec4(0.3f, 0.3f, 0.3f, 1.f);
	desc.diffuse  = Vec4(1.f,  1.f,  1.f,  1.f);
	desc.specular = Vec4(0.1f, 0.1f, 0.1f, 1.f);
	desc.emissive = Vec4(0.f,  0.f,  0.f,  0.f);

	_modelShader = std::make_shared<Shader>(L"../Engine/Shaders/StaticMeshShader.hlsl");

	auto makePreview = [&](bool ok) -> std::shared_ptr<Material>
	{
		auto mat = std::make_shared<Material>();
		mat->SetShader(_modelShader);
		auto& d    = mat->GetMaterialDesc();
		d.ambient  = d.diffuse = ok ? Vec4(0.2f, 0.9f, 0.2f, 0.5f)
		                             : Vec4(0.9f, 0.2f, 0.2f, 0.5f);
		d.specular = d.emissive = Vec4(0.f, 0.f, 0.f, 0.f);
		return mat;
	};

	_previewOk  = makePreview(true);
	_previewBad = makePreview(false);

	CreateAtlasBuffer();
}

void BlockMaterialProvider::CreateAtlasBuffer()
{
	const auto& rects = GET_SINGLE(BlockTable)->GetUVRects();
	if (rects.empty()) return;

	auto* device = GET_SINGLE(Graphics)->GetDevice().Get();

	D3D11_BUFFER_DESC desc = {};
	desc.ByteWidth           = static_cast<UINT>(sizeof(BlockUVRect) * rects.size());
	desc.BindFlags           = D3D11_BIND_SHADER_RESOURCE;
	desc.Usage               = D3D11_USAGE_IMMUTABLE;
	desc.MiscFlags           = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	desc.StructureByteStride = sizeof(BlockUVRect);

	D3D11_SUBRESOURCE_DATA initData = {};
	initData.pSysMem = rects.data();
	CHECK(device->CreateBuffer(&desc, &initData, _atlasRectBuffer.GetAddressOf()));

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format                = DXGI_FORMAT_UNKNOWN;
	srvDesc.ViewDimension         = D3D11_SRV_DIMENSION_BUFFEREX;
	srvDesc.BufferEx.FirstElement = 0;
	srvDesc.BufferEx.NumElements  = static_cast<UINT>(rects.size());
	CHECK(device->CreateShaderResourceView(
		_atlasRectBuffer.Get(), &srvDesc, _atlasRectSRV.GetAddressOf()));

	auto srvVar = _blockShader->GetSRV("g_AtlasRects");
	if (srvVar)
		srvVar->SetResource(_atlasRectSRV.Get());
}