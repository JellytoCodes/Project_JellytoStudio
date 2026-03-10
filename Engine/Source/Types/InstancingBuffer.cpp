#include "Framework.h"
#include "InstancingBuffer.h"

#include "Graphics/Graphics.h"
#include "Pipeline/VertexBuffer.h"

InstancingBuffer::InstancingBuffer()
{
	CreateBuffer(MAX_MESH_INSTANCE);
}

InstancingBuffer::~InstancingBuffer()
{
}

void InstancingBuffer::ClearData()
{
	_data.clear();
}

void InstancingBuffer::AddData(const InstancingData& data)
{
	_data.push_back(data);
}

void InstancingBuffer::PushData()
{
	auto deviceContext = Graphics::Get()->GetDeviceContext();
	const uint32 dataCount = GetCount();
	if (dataCount > _maxCount) CreateBuffer(dataCount);

	D3D11_MAPPED_SUBRESOURCE subResource;

	deviceContext->Map(_instanceBuffer->GetComPtr().Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &subResource);
	{
		::memcpy(subResource.pData, _data.data(), sizeof(InstancingData) * dataCount);
	}
	deviceContext->Unmap(_instanceBuffer->GetComPtr().Get(), 0);

	_instanceBuffer->PushData(deviceContext);
}

void InstancingBuffer::CreateBuffer(const uint32 maxCount)
{
	_maxCount = maxCount;
	_instanceBuffer = std::make_shared<VertexBuffer>();

	std::vector<InstancingData> temp(maxCount);
	_instanceBuffer->Create(temp, 1, true);
}
