#pragma once

#include "Resource.h"

struct MaterialDesc
{
	Color ambient = Color(0.f, 0.f, 0.f, 1.f);
	Color diffuse = Color(1.f, 1.f, 1.f, 1.f);
	Color specular = Color(0.f, 0.f, 0.f, 1.f);
	Color emissive = Color(0.f, 0.f, 0.f, 1.f);
};

class Texture;
class Shader;

class Material : public Resource
{
	using Super = Resource;

public :
	Material();
	virtual ~Material();

	shared_ptr<Shader> GetShader()							{ return _shader; }

	MaterialDesc& GetMaterialDesc()							{ return _desc; }
	shared_ptr<Texture> GetDiffuseMap()						{ return _diffuseMap; }
	shared_ptr<Texture> GetNormalMap()						{ return _normalMap; }
	shared_ptr<Texture> GetSpecularMap()					{ return _specularMap; }
	
	void SetDiffuseMap(shared_ptr<Texture> diffuseMap)		{ _diffuseMap = diffuseMap; }
	void SetNormalMap(shared_ptr<Texture> normalMap)		{ _normalMap = normalMap; }
	void SetSpecularMap(shared_ptr<Texture> specularMap)	{ _specularMap = specularMap; }
	void SetShader(shared_ptr<Shader> shader);

	void Update();

	// shared_ptr<Material> Clone();

private :
	friend class MeshRenderer;

	MaterialDesc									_desc;
	shared_ptr<Shader>								_shader;
	shared_ptr<Texture>								_diffuseMap;
	shared_ptr<Texture>								_normalMap;
	shared_ptr<Texture>								_specularMap;
	
	ComPtr<ID3DX11EffectShaderResourceVariable>		_diffuseEffectBuffer;
	ComPtr<ID3DX11EffectShaderResourceVariable>		_normalEffectBuffer;
	ComPtr<ID3DX11EffectShaderResourceVariable>		_specularEffectBuffer;

};

