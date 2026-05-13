#include "Framework.h"
#include "Texture.h"

#include "Graphics/Graphics.h"
#include "UI/UIManager.h"

Texture::Texture() : Super(ResourceType::Texture) {}
Texture::~Texture()
{
    InvalidateUIHandle();
}

void Texture::Load(const std::wstring& path)
{
    std::wstring ext = std::filesystem::path(path).extension().wstring();
    for (auto& c : ext) c = towlower(c);

    HRESULT hr = E_FAIL;
    if (ext == L".dds")
        hr = DirectX::LoadFromDDSFile(path.c_str(), DirectX::DDS_FLAGS_NONE, nullptr, _img);
    else if (ext == L".tga")
        hr = DirectX::LoadFromTGAFile(path.c_str(), nullptr, _img);
    else
        hr = DirectX::LoadFromWICFile(path.c_str(), DirectX::WIC_FLAGS_NONE, nullptr, _img);

    CHECK(hr);

    CHECK(DirectX::CreateShaderResourceView(
        GET_SINGLE(Graphics)->GetDevice().Get(),
        _img.GetImages(), _img.GetImageCount(),
        _img.GetMetadata(),
        _shaderResourceView.GetAddressOf()));

    _size = { static_cast<float>(_img.GetMetadata().width),
              static_cast<float>(_img.GetMetadata().height) };

    InvalidateUIHandle();
}

ComPtr<ID3D11Texture2D> Texture::GetTexture2D()
{
    if (!_shaderResourceView) return nullptr;
    ComPtr<ID3D11Resource> res;
    _shaderResourceView->GetResource(res.GetAddressOf());
    ComPtr<ID3D11Texture2D> tex;
    res.As(&tex);
    return tex;
}

void Texture::SetSRV(ComPtr<ID3D11ShaderResourceView> srv)
{
    if (_shaderResourceView.Get() == srv.Get()) return;
    InvalidateUIHandle();
    _shaderResourceView = std::move(srv);
}

TextureHandle Texture::GetUIHandle()
{
    if (_uiHandle != kInvalidTextureHandle)
        return _uiHandle;

    if (!_shaderResourceView)
        return kInvalidTextureHandle;

    auto* uiMgr = GET_SINGLE(UIManager);
    if (!uiMgr) return kInvalidTextureHandle;

    _uiHandle = uiMgr->RegisterTexture(_shaderResourceView);
    return _uiHandle;
}

void Texture::InvalidateUIHandle()
{
    if (_uiHandle == kInvalidTextureHandle) return;

    auto* uiMgr = GET_SINGLE(UIManager);
    if (uiMgr) uiMgr->UnregisterTexture(_uiHandle);

    _uiHandle = kInvalidTextureHandle;
}