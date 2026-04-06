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
	_uploaded = false; // 데이터 초기화 → 재업로드 필요
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

	// Map → memcpy → Unmap: dirty 시 1회만 (이후 BindBuffer만 호출)
	auto deviceContext = Graphics::Get()->GetDeviceContext();
	D3D11_MAPPED_SUBRESOURCE subResource;
	deviceContext->Map(_instanceBuffer->GetComPtr().Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &subResource);
	::memcpy(subResource.pData, _data.data(), sizeof(InstancingData) * dataCount);
	deviceContext->Unmap(_instanceBuffer->GetComPtr().Get(), 0);
	_uploaded = true; // 업로드 완료 — 이후 매프레임은 BindBuffer만
}

void InstancingBuffer::BindBuffer()
{
	if (GetCount() == 0) return;
	// slot 1에 인스턴스 버퍼 바인딩 (서브메시 루프마다 호출)
	_instanceBuffer->PushData(Graphics::Get()->GetDeviceContext());
}

void InstancingBuffer::PushData()
{
	// 하위호환 — UploadData + BindBuffer 통합
	UploadData();
	BindBuffer();
}

void InstancingBuffer::CreateBuffer(const uint32 maxCount)
{
	_maxCount = maxCount;
	_instanceBuffer = std::make_shared<VertexBuffer>();

	std::vector<InstancingData> temp(maxCount);
	_instanceBuffer->Create(Graphics::Get()->GetDevice(), temp, 1, true);
}