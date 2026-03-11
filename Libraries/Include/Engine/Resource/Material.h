#pragma once

#include "Resource.h"

class Texture;
class Shader;

class Material : public Resource
{
	using Super = Resource;

public :
	Material();
	virtual ~Material();

	std::shared_ptr<Shader> GetShader()							{ return _shader; }

	MaterialDesc& GetMaterialDesc()								{ return _desc; }
	std::shared_ptr<Texture> GetDiffuseMap()					{ return _diffuseMap; }
	std::shared_ptr<Texture> GetNormalMap()						{ return _normalMap; }
	std::shared_ptr<Texture> GetSpecularMap()					{ return _specularMap; }
	
	void SetDiffuseMap(std::shared_ptr<Texture> diffuseMap)		{ _diffuseMap = diffuseMap; }
	void SetNormalMap(std::shared_ptr<Texture> normalMap)		{ _normalMap = normalMap; }
	void SetSpecularMap(std::shared_ptr<Texture> specularMap)	{ _specularMap = specularMap; }
	void SetShader(std::shared_ptr<Shader> shader);

	void Update();

	std::shared_ptr<Material> Clone();

private :
	friend class MeshRenderer;

	MaterialDesc									_desc;
	std::shared_ptr<Shader>							_shader;
	std::shared_ptr<Texture>						_diffuseMap;
	std::shared_ptr<Texture>						_normalMap;
	std::shared_ptr<Texture>						_specularMap;
	
	ComPtr<ID3DX11EffectShaderResourceVariable>		_diffuseEffectBuffer;
	ComPtr<ID3DX11EffectShaderResourceVariable>		_normalEffectBuffer;
	ComPtr<ID3DX11EffectShaderResourceVariable>		_specularEffectBuffer;
};

