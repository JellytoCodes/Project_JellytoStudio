#include "Framework.h"
#include "InstancingBuffer.h"
#include "DynamicInstancePool.h"
#include "Graphics/Graphics.h"

InstancingBuffer::InstancingBuffer(bool isDynamic)
	: _isDynamic(isDynamic)
{
	CreateBuffers(MAX_MESH_INSTANCE);
}

void InstancingBuffer::CreateBuffers(uint32 maxCount)
{
	_maxCount = maxCount;
	_data.clear();
	_data.reserve(maxCount);

	if (_isDynamic)
	{
		_frameIndex        = 0;
		_currentSlot       = 0;
		_poolElementOffset = 0;
		_dirty             = true;
		_uploaded          = false;
		return;
	}

	auto* device = GET_SINGLE(Graphics)->GetDevice().Get();

	D3D11_BUFFER_DESC desc = {};
	desc.ByteWidth      = sizeof(InstancingData) * maxCount;
	desc.BindFlags      = D3D11_BIND_VERTEX_BUFFER;
	desc.Usage          = D3D11_USAGE_DEFAULT;
	desc.CPUAccessFlags = 0;

	_ringBuffers[0].Reset();
	CHECK(device->CreateBuffer(&desc, nullptr, _ringBuffers[0].GetAddressOf()));

	_frameIndex        = 0;
	_currentSlot       = 0;
	_poolElementOffset = 0;
	_dirty             = true;
	_uploaded          = false;
}

void InstancingBuffer::PromoteToDynamic()
{
	if (_isDynamic) return;

	_isDynamic = true;
	for (uint32 i = 0; i < kRingCount; ++i)
		_ringBuffers[i].Reset();

	_frameIndex        = 0;
	_currentSlot       = 0;
	_poolElementOffset = 0;
	_dirty             = true;
	_uploaded          = false;

	if (!_data.empty())
		UploadData();
}

void InstancingBuffer::ClearData()
{
	_data.clear();
	_dirty    = true;
	_uploaded = false;
}

void InstancingBuffer::AddData(const InstancingData& data)
{
	_data.push_back(data);
	_dirty = true;
}

void InstancingBuffer::SetData(const InstancingData* data, uint32 count)
{
	_data.resize(count);
	if (count > 0)
		::memcpy(_data.data(), data, sizeof(InstancingData) * count);
	_dirty    = true;
	_uploaded = false;
}

void InstancingBuffer::UploadData()
{
	const uint32 count = GetCount();
	if (count == 0 || !_dirty) return;

	if (_isDynamic)
	{
		_poolElementOffset =
			GET_SINGLE(DynamicInstancePool)->Append(_data.data(), count);
		_dirty    = false;
		_uploaded = true;
		return;
	}

	if (count > _maxCount)
	{
		std::vector<InstancingData> saved = std::move(_data);

		const uint32 newCount = max(_maxCount * 2u, count);
		_maxCount = newCount;

		auto* device = GET_SINGLE(Graphics)->GetDevice().Get();

		D3D11_BUFFER_DESC desc = {};
		desc.ByteWidth      = sizeof(InstancingData) * newCount;
		desc.BindFlags      = D3D11_BIND_VERTEX_BUFFER;
		desc.Usage          = D3D11_USAGE_DEFAULT;
		desc.CPUAccessFlags = 0;

		_ringBuffers[0].Reset();
		CHECK(device->CreateBuffer(&desc, nullptr, _ringBuffers[0].GetAddressOf()));

		_data = std::move(saved);
		_data.reserve(newCount);
		_currentSlot = 0;
	}

	auto* ctx = GET_SINGLE(Graphics)->GetDeviceContext().Get();
	ctx->UpdateSubresource(
		_ringBuffers[0].Get(),
		0, nullptr,
		_data.data(),
		sizeof(InstancingData) * count,
		0);

	_currentSlot = 0;
	_dirty       = false;
	_uploaded    = true;
}

void InstancingBuffer::BindBuffer() const
{
	if (!_uploaded || GetCount() == 0) return;

	if (_isDynamic)
	{
		GET_SINGLE(DynamicInstancePool)->BindSlice(_poolElementOffset);
		return;
	}

	auto* ctx = GET_SINGLE(Graphics)->GetDeviceContext().Get();

	const UINT stride = sizeof(InstancingData);
	const UINT offset = 0;

	ctx->IASetVertexBuffers(
		1, 1,
		_ringBuffers[_currentSlot].GetAddressOf(),
		&stride,
		&offset);
}

void InstancingBuffer::PushData()
{
	UploadData();
	BindBuffer();
}