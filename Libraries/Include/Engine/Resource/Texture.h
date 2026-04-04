#pragma once
#include "Resource.h"

class Texture : public Resource
{
	using Super = Resource;

public :
	Texture();
	virtual ~Texture();

	ComPtr<ID3D11ShaderResourceView>	GetComPtr() { return _shaderResourceView; }

	virtual void						Load(const std::wstring& path) override;
	
	ComPtr<ID3D11Texture2D>				GetTexture2D();
	void								SetSRV(ComPtr<ID3D11ShaderResourceView> srv) { _shaderResourceView = srv; }

	Vec2								GetSize() const { return _size; }

	const DirectX::ScratchImage& GetInfo() { return _img; }

private:
	ComPtr<ID3D11ShaderResourceView> _shaderResourceView;
	Vec2 _size = { 0.f, 0.f };
	DirectX::ScratchImage _img = {};
};

