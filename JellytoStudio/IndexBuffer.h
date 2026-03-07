#pragma once

class IndexBuffer
{
public:
	IndexBuffer()		{ }
	~IndexBuffer()		{ }

	ComPtr<ID3D11Buffer> GetComPtr()	{ return _indexBuffer; }
	uint32 GetStride() const			{ return _stride; }
	uint32 GetOffset() const			{ return _offset; }
	uint32 GetCount() const				{ return _count; }

	void Create(const ComPtr<ID3D11Device>& device, const vector<uint32>& indices);

	void PushData(const ComPtr<ID3D11DeviceContext>& deviceContext)
	{
		deviceContext->IASetIndexBuffer(_indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
	}

private:
	ComPtr<ID3D11Buffer>	_indexBuffer;

	uint32					_stride	= 0;
	uint32					_offset	= 0;
	uint32					_count	= 0;
};