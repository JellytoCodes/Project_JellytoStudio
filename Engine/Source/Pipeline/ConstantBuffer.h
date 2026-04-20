#pragma once

template<typename T>
class ConstantBuffer
{
public:
	ConstantBuffer() = default;
	~ConstantBuffer() = default;

	ComPtr<ID3D11Buffer> GetComPtr()	{ return _constantBuffer; }

	void Create(const ComPtr<ID3D11Device>& device);
	void CopyData(const ComPtr<ID3D11DeviceContext>& deviceContext, const T& data);

private:
	ComPtr<ID3D11Buffer>	_constantBuffer;
};

template <typename T>
void ConstantBuffer<T>::Create(const ComPtr<ID3D11Device>& device)
{
	D3D11_BUFFER_DESC desc = {};
	desc.Usage = D3D11_USAGE_DYNAMIC; // CPU_Write + GPU_Read
	desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	desc.ByteWidth = sizeof(T);
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	HRESULT hr = device->CreateBuffer(&desc, nullptr, _constantBuffer.GetAddressOf());
	CHECK(hr);
}

template <typename T>
void ConstantBuffer<T>::CopyData(const ComPtr<ID3D11DeviceContext>& deviceContext, const T& data)
{
	D3D11_MAPPED_SUBRESOURCE subResource = {};

	deviceContext->Map(_constantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &subResource);
	::memcpy(subResource.pData, &data, sizeof(data));
	deviceContext->Unmap(_constantBuffer.Get(), 0);
}