
#include "Framework.h"
#include "Material.h"
#include "Resource/Texture.h"
#include "Pipeline/Shader.h"

Material::Material()
	: Super(ResourceType::Material)
{

}

Material::~Material()
{

}

void Material::SetShader(std::shared_ptr<Shader> shader)
{
	_shader					= shader;

	_diffuseEffectBuffer	= shader->GetSRV("DiffuseMap");
	_normalEffectBuffer		= shader->GetSRV("NormalMap");
	_specularEffectBuffer	= shader->GetSRV("SpecularMap");
}

void Material::Update()
{
	if (_shader == nullptr) return;

	_shader->PushMaterialData(_desc);

	if (_diffuseMap)	_diffuseEffectBuffer->SetResource(_diffuseMap->GetComPtr().Get());
	if (_normalMap)		_normalEffectBuffer->SetResource(_normalMap->GetComPtr().Get());
	if (_specularMap)	_specularEffectBuffer->SetResource(_specularMap->GetComPtr().Get());
}

std::shared_ptr<Material> Material::Clone()
{
	std::shared_ptr<Material> material = std::make_shared<Material>();

	material->_desc = _desc;
	material->_shader = _shader;
	material->_diffuseMap = _diffuseMap;
	material->_normalMap = _normalMap;
	material->_specularMap = _specularMap;
	material->_diffuseEffectBuffer = _diffuseEffectBuffer;
	material->_normalEffectBuffer = _normalEffectBuffer;
	material->_specularEffectBuffer = _specularEffectBuffer;

	return material;
}