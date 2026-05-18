#include "Framework.h"
#include "Material.h"

#include "Resource/Texture.h"
#include "Pipeline/Shader.h"
#include "Resource/TextureArray.h"

Material::Material()
    : Super(ResourceType::Material)
{}

Material::~Material() {}

void Material::SetShader(std::shared_ptr<Shader> shader)
{
    _shader = shader;
    if (!shader) return;

    _diffuseEffectBuffer = shader->GetSRV("DiffuseMap");
    _normalEffectBuffer = shader->GetSRV("NormalMap");
    _specularEffectBuffer = shader->GetSRV("SpecularMap");

    
    _textureArrayEffectBuffer = shader->HasVariable("g_BlockAtlas") ? shader->GetSRV("g_BlockAtlas") : nullptr;
}

void Material::Update()
{
    if (_shader == nullptr) return;
    _shader->PushMaterialData(_desc);

    if (_diffuseEffectBuffer)
        _diffuseEffectBuffer->SetResource(_diffuseMap ? _diffuseMap->GetComPtr().Get() : nullptr);

    if (_normalEffectBuffer)
        _normalEffectBuffer->SetResource(_normalMap ? _normalMap->GetComPtr().Get() : nullptr);

    if (_specularEffectBuffer)
        _specularEffectBuffer->SetResource(_specularMap ? _specularMap->GetComPtr().Get() : nullptr);

    if (_textureArrayEffectBuffer)
    {
        if (_textureArray)
            _textureArrayEffectBuffer->SetResource(_textureArray->GetSRV().Get());
        else if (_diffuseMap)
            _textureArrayEffectBuffer->SetResource(_diffuseMap->GetComPtr().Get());
    }
}

std::unique_ptr<Material> Material::Clone() const
{
    auto material = std::make_unique<Material>();
    material->_desc                      = _desc;
    material->_shader                    = _shader;
    material->_diffuseMap                = _diffuseMap;
    material->_normalMap                 = _normalMap;
    material->_specularMap               = _specularMap;
    material->_diffuseEffectBuffer       = _diffuseEffectBuffer;
    material->_normalEffectBuffer        = _normalEffectBuffer;
    material->_specularEffectBuffer      = _specularEffectBuffer;
    material->_textureArrayEffectBuffer  = _textureArrayEffectBuffer;
    material->_textureArray              = _textureArray;         
    return material;
}