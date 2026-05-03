#include "Framework.h"
#include "TextureArray.h"

#include "Graphics/Graphics.h"

std::shared_ptr<TextureArray> TextureArray::Create(
    const std::wstring* paths, uint32 count)
{
    assert(paths && count > 0);

    auto* device = GET_SINGLE(Graphics)->GetDevice().Get();
    auto* ctx    = GET_SINGLE(Graphics)->GetDeviceContext().Get();

    std::vector<ComPtr<ID3D11Texture2D>> slices(count);
    D3D11_TEXTURE2D_DESC slice0Desc = {};

    for (uint32 i = 0; i < count; ++i)
    {
        DirectX::ScratchImage img;
        HRESULT hr = DirectX::LoadFromWICFile(
            paths[i].c_str(), DirectX::WIC_FLAGS_NONE, nullptr, img);

        if (SUCCEEDED(hr))
        {
            ComPtr<ID3D11Resource> res;
            hr = DirectX::CreateTexture(device,
                img.GetImages(), img.GetImageCount(),
                img.GetMetadata(), res.GetAddressOf());
            if (SUCCEEDED(hr))
                hr = res.As(&slices[i]);
        }

        if (FAILED(hr))
        {
            assert(i > 0 && "첫 슬라이스 로드 실패");
            slices[i] = slices[i - 1];
            continue;
        }

        if (i == 0) slices[0]->GetDesc(&slice0Desc);
    }

    D3D11_TEXTURE2D_DESC arrDesc  = slice0Desc;
    arrDesc.ArraySize             = count;
    arrDesc.Usage                 = D3D11_USAGE_DEFAULT;
    arrDesc.BindFlags             = D3D11_BIND_SHADER_RESOURCE;
    arrDesc.CPUAccessFlags        = 0;
    arrDesc.MiscFlags             = 0;

    auto ta = std::make_shared<TextureArray>();
    HRESULT hr = device->CreateTexture2D(&arrDesc, nullptr,
        ta->_tex.GetAddressOf());
    assert(SUCCEEDED(hr));

    for (uint32 i = 0; i < count; ++i)
        for (uint32 mip = 0; mip < slice0Desc.MipLevels; ++mip)
            ctx->CopySubresourceRegion(
                ta->_tex.Get(),
                D3D11CalcSubresource(mip, i, slice0Desc.MipLevels),
                0, 0, 0,
                slices[i].Get(),
                D3D11CalcSubresource(mip, 0, slice0Desc.MipLevels),
                nullptr);

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc        = {};
    srvDesc.Format                                 = arrDesc.Format;
    srvDesc.ViewDimension                          = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
    srvDesc.Texture2DArray.MostDetailedMip         = 0;
    srvDesc.Texture2DArray.MipLevels               = arrDesc.MipLevels;
    srvDesc.Texture2DArray.FirstArraySlice         = 0;
    srvDesc.Texture2DArray.ArraySize               = count;

    hr = device->CreateShaderResourceView(
        ta->_tex.Get(), &srvDesc, ta->_srv.GetAddressOf());
    assert(SUCCEEDED(hr));

    return ta;
}