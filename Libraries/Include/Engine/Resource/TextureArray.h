#pragma once
#include "Resource.h"

class TextureArray : public Resource
{
    using Super = Resource;
public:
    TextureArray() : Super(ResourceType::TextureArray) {}
    ~TextureArray() = default;

    static std::shared_ptr<TextureArray> Create(
        const std::wstring* paths, uint32 count);

    ComPtr<ID3D11ShaderResourceView> GetSRV() const { return _srv; }

private:
    ComPtr<ID3D11ShaderResourceView> _srv;
    ComPtr<ID3D11Texture2D>          _tex;
};