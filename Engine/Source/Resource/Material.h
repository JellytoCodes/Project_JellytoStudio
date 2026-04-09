#pragma once

#include "Resource.h"

class Texture;
class Shader;

class Material : public Resource
{
	using Super = Resource;

public:
	Material();
	virtual ~Material();

	std::shared_ptr<Shader>  GetShader()      { return _shader; }

	MaterialDesc& GetMaterialDesc() { return _desc; }

	// 경우에 따라 RawPointer로 넘겨야 하는 경우도 발생할 수 있음으로 추후 변경 사항이 다수 발생할 수 있음
	std::shared_ptr<Texture> GetDiffuseMap()  { return _diffuseMap; }
	std::shared_ptr<Texture> GetNormalMap()   { return _normalMap; }
	std::shared_ptr<Texture> GetSpecularMap() { return _specularMap; }

	void SetDiffuseMap(std::shared_ptr<Texture> diffuseMap)   { _diffuseMap  = diffuseMap; }
	void SetNormalMap(std::shared_ptr<Texture> normalMap)     { _normalMap   = normalMap; }
	void SetSpecularMap(std::shared_ptr<Texture> specularMap) { _specularMap = specularMap; }
	void SetShader(std::shared_ptr<Shader> shader);

	void Update();

	std::unique_ptr<Material> Clone() const;

private:
	friend class MeshRenderer;

	MaterialDesc _desc;

	std::shared_ptr<Shader>  _shader      = nullptr;
	std::shared_ptr<Texture> _diffuseMap  = nullptr;
	std::shared_ptr<Texture> _normalMap   = nullptr;
	std::shared_ptr<Texture> _specularMap = nullptr;

	ComPtr<ID3DX11EffectShaderResourceVariable> _diffuseEffectBuffer;
	ComPtr<ID3DX11EffectShaderResourceVariable> _normalEffectBuffer;
	ComPtr<ID3DX11EffectShaderResourceVariable> _specularEffectBuffer;
};
