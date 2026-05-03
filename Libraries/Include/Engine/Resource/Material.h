#pragma once
#include "Resource.h"

class Texture;
class Shader;
class TextureArray;

class Material : public Resource
{
    using Super = Resource;

public:
    Material();
    virtual ~Material();

    std::shared_ptr<Shader>         GetShader()                                         { return _shader; }
    MaterialDesc&                   GetMaterialDesc()                                   { return _desc;   }

    std::shared_ptr<Texture>        GetDiffuseMap()                                     { return _diffuseMap;  }
    std::shared_ptr<Texture>        GetNormalMap()                                      { return _normalMap;   }
    std::shared_ptr<Texture>        GetSpecularMap()                                    { return _specularMap; }

    void                            SetDiffuseMap (std::shared_ptr<Texture> t)          { _diffuseMap  = t; }
    void                            SetNormalMap  (std::shared_ptr<Texture> t)          { _normalMap   = t; }
    void                            SetSpecularMap(std::shared_ptr<Texture> t)          { _specularMap = t; }

    void                            SetShader(std::shared_ptr<Shader> shader);
    void                            Update();

	void                            SetTextureArray(std::shared_ptr<TextureArray> ta)   { _textureArray = std::move(ta); }

    std::unique_ptr<Material>       Clone() const;

private:
    friend class MeshRenderer;

    MaterialDesc                                    _desc;

    std::shared_ptr<Shader>                         _shader;
    std::shared_ptr<Texture>                        _diffuseMap;
    std::shared_ptr<Texture>                        _normalMap;
    std::shared_ptr<Texture>                        _specularMap;

    ComPtr<ID3DX11EffectShaderResourceVariable>     _diffuseEffectBuffer;
    ComPtr<ID3DX11EffectShaderResourceVariable>     _normalEffectBuffer;
    ComPtr<ID3DX11EffectShaderResourceVariable>     _specularEffectBuffer;

    ComPtr<ID3DX11EffectShaderResourceVariable>     _textureArrayEffectBuffer;
    std::shared_ptr<TextureArray>                   _textureArray;
};