#pragma once

class Material;
class Shader;

class BlockMaterialProvider
{
	DECLARE_SINGLE(BlockMaterialProvider)

public:
	void Init();

	std::shared_ptr<Shader>   GetBlockShader()   const { return _blockShader;   }
	std::shared_ptr<Material> GetBlockMaterial() const { return _blockMaterial; }
	std::shared_ptr<Shader>   GetModelShader()   const { return _modelShader;   }
	std::shared_ptr<Material> GetPreviewMat(bool ok) const { return ok ? _previewOk : _previewBad; }

private:
	void CreateAtlasBuffer();

	std::shared_ptr<Shader>   _blockShader;
	std::shared_ptr<Material> _blockMaterial;
	std::shared_ptr<Shader>   _modelShader;
	std::shared_ptr<Material> _previewOk;
	std::shared_ptr<Material> _previewBad;

	ComPtr<ID3D11Buffer>             _atlasRectBuffer;
	ComPtr<ID3D11ShaderResourceView> _atlasRectSRV;
};