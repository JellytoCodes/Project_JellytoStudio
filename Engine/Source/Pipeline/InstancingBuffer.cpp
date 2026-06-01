#include "Framework.h"
#include "InstancingBuffer.h"
#include "DynamicInstancePool.h"
#include "Graphics/Graphics.h"

uint32 InstancingBuffer::NextTier(uint32 count)
{
    if (count <= kTierSmall)  return kTierSmall;
    if (count <= kTierMedium) return kTierMedium;
    if (count <= kTierLarge)  return kTierLarge;
    return kTierMax;
}

void InstancingBuffer::AllocStaticBuffer(uint32 maxCount)
{
    auto* device = GET_SINGLE(Graphics)->GetDevice().Get();
    if (device == nullptr || maxCount == 0) return;

    D3D11_BUFFER_DESC desc = {};
    desc.ByteWidth = sizeof(InstancingData) * maxCount;
    desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    desc.Usage = D3D11_USAGE_DYNAMIC;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    _ringBuffers[0].Reset();
    CHECK(device->CreateBuffer(&desc, nullptr, _ringBuffers[0].GetAddressOf()));

    _maxCount = maxCount;
    _currentSlot = 0;
}

InstancingBuffer::InstancingBuffer(bool isDynamic)
    : _isDynamic(isDynamic)
{
    _maxCount = 0;
    _frameIndex = 0;
    _currentSlot = 0;
    _dirty = true;
    _uploaded = false;
}

void InstancingBuffer::PromoteToDynamic()
{
    if (_isDynamic) return;

    _isDynamic = true;
    for (uint32 i = 0; i < kRingCount; ++i)
        _ringBuffers[i].Reset();

    _maxCount = 0;
    _frameIndex = 0;
    _currentSlot = 0;
    _poolElementOffset = 0;
    _dirty = true;
    _uploaded = false;

    if (!_data.empty())
        UploadData();
}

void InstancingBuffer::ClearData()
{
    _data.clear();
    _pendingPtr = nullptr;
    _pendingCount = 0;
    _dirty = true;
    _uploaded = false;
}

void InstancingBuffer::AddData(const InstancingData& data)
{
    _data.push_back(data);
    _dirty = true;
}

void InstancingBuffer::SetData(const InstancingData* data, uint32 count)
{
    if (_isDynamic)
    {
        _pendingPtr = data;
        _pendingCount = count;
        _dirty = true;
        _uploaded = false;
        return;
    }

    _data.resize(count);
    if (count > 0 && data != nullptr)
        ::memcpy(_data.data(), data, sizeof(InstancingData) * count);
    _dirty = true;
    _uploaded = false;
}

void InstancingBuffer::UploadData()
{
    if (!_dirty) return;

    if (_isDynamic)
    {
        if (_pendingPtr && _pendingCount > 0)
        {
            _poolElementOffset = GET_SINGLE(DynamicInstancePool)->Append(_pendingPtr, _pendingCount);
            _pendingPtr = nullptr;
        }
        else
        {
            const uint32 count = static_cast<uint32>(_data.size());
            if (count == 0)
            {
                _dirty = false;
                _uploaded = false;
                return;
            }
            _poolElementOffset = GET_SINGLE(DynamicInstancePool)->Append(_data.data(), count);
            _pendingCount = count;
        }
        _dirty = false;
        _uploaded = true;
        return;
    }

    const uint32 count = static_cast<uint32>(_data.size());
    if (count == 0)
    {
        _dirty = false;
        _uploaded = false;
        return;
    }

    if (!_ringBuffers[0] || count > _maxCount)
        AllocStaticBuffer(NextTier(count));

    auto* ctx = GET_SINGLE(Graphics)->GetDeviceContext().Get();
    if (ctx == nullptr || _ringBuffers[0] == nullptr) return;

    D3D11_MAPPED_SUBRESOURCE mapped = {};
    HRESULT hr = ctx->Map(_ringBuffers[0].Get(), 0,
        D3D11_MAP_WRITE_DISCARD, 0, &mapped);

    if (SUCCEEDED(hr))
    {
        ::memcpy(mapped.pData, _data.data(), sizeof(InstancingData) * count);
        ctx->Unmap(_ringBuffers[0].Get(), 0);
    }

    _currentSlot = 0;
    _dirty = false;
    _uploaded = SUCCEEDED(hr);
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
    if (ctx == nullptr || _ringBuffers[_currentSlot] == nullptr) return;

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
