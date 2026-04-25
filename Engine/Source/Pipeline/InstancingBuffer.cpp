#include "Framework.h"
#include "InstancingBuffer.h"
#include "Graphics/Graphics.h"

InstancingBuffer::InstancingBuffer(bool isDynamic)
    : _isDynamic(isDynamic)
{
    CreateRingBuffers(MAX_MESH_INSTANCE);
}

void InstancingBuffer::CreateRingBuffers(uint32 maxCount)
{
    _maxCount = maxCount;

    _data.clear();
    _data.reserve(maxCount);

    auto* device = GET_SINGLE(Graphics)->GetDevice().Get();

    D3D11_BUFFER_DESC desc = {};
    desc.ByteWidth      = sizeof(InstancingData) * maxCount;
    desc.Usage          = D3D11_USAGE_DYNAMIC;
    desc.BindFlags      = D3D11_BIND_VERTEX_BUFFER;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    for (uint32 i = 0; i < kRingCount; i++)
    {
        _ringBuffers[i].Reset();
        const HRESULT hr = device->CreateBuffer(&desc, nullptr, _ringBuffers[i].GetAddressOf());
        CHECK(hr);
    }

    _frameIndex  = 0;
    _currentSlot = 0;
    _dirty       = true;
    _uploaded    = false;
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

void InstancingBuffer::UploadData()
{
    const uint32 count = GetCount();
    if (count == 0) return;

    if (!_isDynamic && !_dirty) return;

    if (count > _maxCount)
    {
        const uint32 newCount = max(_maxCount * 2, count);
        CreateRingBuffers(newCount);
    }

    _currentSlot = _frameIndex % kRingCount;
    ++_frameIndex;

    auto* ctx = GET_SINGLE(Graphics)->GetDeviceContext().Get();

    D3D11_MAPPED_SUBRESOURCE mapped = {};
    const HRESULT hr = ctx->Map(
        _ringBuffers[_currentSlot].Get(),
        0,
        D3D11_MAP_WRITE_DISCARD,
        0,
        &mapped
    );
    CHECK(hr);

    ::memcpy(mapped.pData, _data.data(), sizeof(InstancingData) * count);

    ctx->Unmap(_ringBuffers[_currentSlot].Get(), 0);

    _dirty    = false;
    _uploaded = true;
}

void InstancingBuffer::BindBuffer() const
{
    if (!_uploaded || GetCount() == 0) return;

    auto* ctx = GET_SINGLE(Graphics)->GetDeviceContext().Get();

    const UINT stride = sizeof(InstancingData);
    const UINT offset = 0;

    ctx->IASetVertexBuffers(1, 1, _ringBuffers[_currentSlot].GetAddressOf(), &stride, &offset);
}

void InstancingBuffer::PushData()
{
    UploadData();
    BindBuffer();
}