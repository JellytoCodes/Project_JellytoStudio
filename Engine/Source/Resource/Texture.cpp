#include "Framework.h"
#include "Texture.h"

#include "Graphics/Graphics.h"
#include "UI/UIManager.h"

Texture::Texture()
	: Super(ResourceType::Texture)
{

}

Texture::~Texture()
{
	InvalidateUIHandle();
}

void Texture::Load(const std::wstring& path)
{
	DirectX::TexMetadata md;
	HRESULT hr = ::LoadFromWICFile(path.c_str(), WIC_FLAGS_NONE, &md, _img);
	CHECK(hr);

	hr = ::CreateShaderResourceView(GET_SINGLE(Graphics)->GetDevice().Get(), _img.GetImages(), _img.GetImageCount(), md, _shaderResourceView.GetAddressOf());
	CHECK(hr);

	_size.x = static_cast<float>(md.width);
	_size.y = static_cast<float>(md.height);

	InvalidateUIHandle();
}

ComPtr<ID3D11Texture2D> Texture::GetTexture2D()
{
	ComPtr<ID3D11Texture2D> texture;
	_shaderResourceView->GetResource((ID3D11Resource**)texture.GetAddressOf());

	return texture;
}

TextureHandle Texture::GetUIHandle()
{
	if (_uiHandle != kInvalidTextureHandle)
		return _uiHandle;

	if (!_shaderResourceView)
		return kInvalidTextureHandle;

	_uiHandle = GET_SINGLE(UIManager)->RegisterTexture(_shaderResourceView);
	return _uiHandle;
}

void Texture::InvalidateUIHandle()
{
	if (_uiHandle == kInvalidTextureHandle) return;
	GET_SINGLE(UIManager)->UnregisterTexture(_uiHandle);
	_uiHandle = kInvalidTextureHandle;
}