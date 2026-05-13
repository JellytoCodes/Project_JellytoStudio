#include "pch.h"
#include "BlockMaterialProvider.h"

#include "Data/BlockTable.h"
#include "Resource/Managers/ResourceManager.h"
#include "Resource/Material.h"
#include "Pipeline/Shader.h"

void BlockMaterialProvider::Init()
{
	auto* table = GET_SINGLE(BlockTable);

	auto paletteTex = GET_SINGLE(ResourceManager)->GetOrAddTexture(L"BlockPalette", table->GetPalettePath());

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
}