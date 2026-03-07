
#include "Framework.h"
#include "Material.h"
#include "Pipeline/Shader.h"

Material::Material()
	: Super(ResourceType::Material)
{

}

Material::~Material()
{

}

void Material::SetShader(shared_ptr<Shader> shader)
{
	_shader					= shader;

	_diffuseEffectBuffer	= shader->GetSRV("DiffuseMap");
	_normalEffectBuffer		= shader->GetSRV("NormalMap");
	_specularEffectBuffer	= shader->GetSRV("SpecularMap");
}

void Material::Update()
{

}
