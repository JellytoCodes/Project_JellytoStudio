#include "Framework.h"
#include "InstancingBuffer.h"

#include "Graphics/Graphics.h"
#include "Pipeline/VertexBuffer.h"

InstancingBuffer::InstancingBuffer()
{
	CreateBuffer(MAX_MESH_INSTANCE);
}

InstancingBuffer::~InstancingBuffer() {}

void InstancingBuffer::ClearData()
{
	_data.clear();
	_uploaded = false;
}

void InstancingBuffer::AddData(const InstancingData& data)
{
	_data.push_back(data);
}

void InstancingBuffer::UploadData()
{
	const uint32 dataCount = GetCount();
	if (dataCount == 0) return;

	if (dataCount > _maxCount)
	{
		uint32 newCount = _maxCount * 2;
		if (newCount < dataCount) newCount = dataCount;
		CreateBuffer(newCount);
	}

	auto deviceContext = GET_SINGLE(Graphics)->GetDeviceContext();
	D3D11_MAPPED_SUBRESOURCE subResource;
	deviceContext->Map(_instanceBuffer->GetComPtr().Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &subResource);
	::memcpy(subResource.pData, _data.data(), sizeof(InstancingData) * dataCount);
	deviceContext->Unmap(_instanceBuffer->GetComPtr().Get(), 0);
	_uploaded = true;
}

void InstancingBuffer::BindBuffer()
{
	if (GetCount() == 0) return;
	_instanceBuffer->PushData(GET_SINGLE(Graphics)->GetDeviceContext());
}

void InstancingBuffer::PushData()
{
	UploadData();
	BindBuffer();
}

void InstancingBuffer::CreateBuffer(uint32 maxCount)
{
	_maxCount = maxCount;
	_instanceBuffer = std::make_unique<VertexBuffer>();

	std::vector<InstancingData> temp(maxCount);
	_instanceBuffer->Create(GET_SINGLE(Graphics)->GetDevice(), temp, 1, true);
}
