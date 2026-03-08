#pragma once

class VertexBuffer
{
public:
	VertexBuffer()		{ }
	~VertexBuffer()		{ }

	ComPtr<ID3D11Buffer> GetComPtr()	{ return _vertexBuffer; }

	uint32 GetStride() const			{ return _stride; }
	uint32 GetOffset() const			{ return _offset; }
	uint32 GetCount() const				{ return _count; }
	uint32 GetSlot() const				{ return _slot; }

	template<typename T>
	void Create(const ComPtr<ID3D11Device>& device, const std::vector<T>& vertices, uint32 slot = 0, bool cpuWrite = false, bool gpuWrite = false);

	void PushData(const ComPtr<ID3D11DeviceContext>& deviceContext)
	{
		deviceContext->IASetVertexBuffers(_slot, 1, _vertexBuffer.GetAddressOf(), &_stride, &_offset);
	}

private:
	ComPtr<ID3D11Buffer> _vertexBuffer;

	uint32 _stride = 0;
	uint32 _offset = 0;
	uint32 _count = 0;

	uint32 _slot = 0;
	bool _cpuWrite = false;
	bool _gpuWrite = false;
};

template <typename T>
void VertexBuffer::Create(const ComPtr<ID3D11Device>& device, const std::vector<T>& vertices, uint32 slot, bool cpuWrite, bool gpuWrite)
{
	_stride = sizeof(T);
	_count = static_cast<uint32>(vertices.size());

	_slot = slot;
	_cpuWrite = cpuWrite;
	_gpuWrite = gpuWrite;

	D3D11_BUFFER_DESC desc;
	ZeroMemory(&desc, sizeof(desc));
	desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	desc.ByteWidth = (uint32)(_stride * _count);

	if (cpuWrite == false && gpuWrite == false)
	{
		desc.Usage = D3D11_USAGE_IMMUTABLE;
	}
	else if (cpuWrite == true && gpuWrite == false)
	{
		desc.Usage = D3D11_USAGE_DYNAMIC;
		desc.CPUAccessFlags = D3D10_CPU_ACCESS_WRITE;
	}
	else if (cpuWrite == false && gpuWrite == true)
	{
		desc.Usage = D3D11_USAGE_DEFAULT;
	}
	else
	{
		desc.Usage = D3D11_USAGE_STAGING;
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;
	}

	D3D11_SUBRESOURCE_DATA data;
	ZeroMemory(&data, sizeof(data));
	data.pSysMem = vertices.data();

	HRESULT hr = device->CreateBuffer(&desc, &data, _vertexBuffer.GetAddressOf());
	CHECK(hr);
}

