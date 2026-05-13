#pragma once

#include "Resource.h"
#include "UI/UITypes.h"

class Texture : public Resource
{
    using Super = Resource;

public:
    Texture();
    virtual ~Texture();

    ComPtr<ID3D11ShaderResourceView> GetComPtr() { return _shaderResourceView; }
    void SetSRV(ComPtr<ID3D11ShaderResourceView> srv);

    ComPtr<ID3D11Texture2D> GetTexture2D();
    const DirectX::ScratchImage& GetInfo() { return _img; }

    virtual void Load(const std::wstring& path) override;
    Vec2         GetSize() const { return _size; }

    TextureHandle GetUIHandle();

    void          InvalidateUIHandle();

private:
    ComPtr<ID3D11ShaderResourceView> _shaderResourceView;
    Vec2                             _size     = { 0.f, 0.f };
    DirectX::ScratchImage            _img      = {};

    TextureHandle                    _uiHandle = kInvalidTextureHandle;
};