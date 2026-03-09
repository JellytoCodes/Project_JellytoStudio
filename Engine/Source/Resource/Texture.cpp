#include "Framework.h"
#include "Texture.h"

#include "Graphics/Graphics.h"

Texture::Texture()
	: Super(ResourceType::Texture)
{

}

Texture::~Texture()
{

}

void Texture::Load(const std::wstring& path)
{
	DirectX::TexMetadata md;
	HRESULT hr = ::LoadFromWICFile(path.c_str(), WIC_FLAGS_NONE, &md, _img);
	CHECK(hr);

	hr = ::CreateShaderResourceView(Graphics::Get()->GetDevice().Get(), _img.GetImages(), _img.GetImageCount(), md, _shaderResourceView.GetAddressOf());
	CHECK(hr);

	_size.x = static_cast<float>(md.width);
	_size.y = static_cast<float>(md.height);
}

ComPtr<ID3D11Texture2D> Texture::GetTexture2D()
{
	ComPtr<ID3D11Texture2D> texture;
	_shaderResourceView->GetResource((ID3D11Resource**)texture.GetAddressOf());

	return texture;
}
