#include "Framework.h"
#include "DynamicInstancePool.h"
#include "Graphics/Graphics.h"

void DynamicInstancePool::Init()
{
    auto* device = GET_SINGLE(Graphics)->GetDevice().Get();
    if (device == nullptr) return;

    D3D11_BUFFER_DESC desc = {};
    desc.ByteWidth = sizeof(InstancingData) * kMaxInstances;
    desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    desc.Usage = D3D11_USAGE_DYNAMIC;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    for (uint32 i = 0; i < kRingCount; ++i)
    {
        _ringBuffers[i].Reset();
        CHECK(device->CreateBuffer(&desc, nullptr, _ringBuffers[i].GetAddressOf()));
    }

    _frameIndex = 0;
    _currentSlot = 0;
    _writeOffset = 0;
    _mappedPtr = nullptr;
    _ready = true;
}

void DynamicInstancePool::BeginFrame()
{
    if (!_ready) return;
    if (_mappedPtr != nullptr)
        EndFrame();

    _currentSlot = _frameIndex % kRingCount;
    ++_frameIndex;
    _writeOffset = 0;

    auto* ctx = GET_SINGLE(Graphics)->GetDeviceContext().Get();
    if (ctx == nullptr) return;

    D3D11_MAPPED_SUBRESOURCE mapped = {};
    CHECK(ctx->Map(_ringBuffers[_currentSlot].Get(), 0,
        D3D11_MAP_WRITE_DISCARD, 0, &mapped));
    _mappedPtr = static_cast<uint8_t*>(mapped.pData);
}

uint32 DynamicInstancePool::Append(const InstancingData* data, uint32 count)
{
    if (!_ready || data == nullptr || count == 0) return 0;

    assert(_mappedPtr && "DynamicInstancePool::Append requires BeginFrame().");
    if (!_mappedPtr) return 0;

    const uint32 required = _writeOffset + count;
    if (required > kMaxInstances)
    {
        assert(false && "DynamicInstancePool overflow. Increase kMaxInstances.");
        return _writeOffset;
    }

    ::memcpy(_mappedPtr + _writeOffset * sizeof(InstancingData),
        data,
        sizeof(InstancingData) * count);

    const uint32 returnedOffset = _writeOffset;
    _writeOffset += count;
    return returnedOffset;
}

void DynamicInstancePool::EndFrame()
{
    if (!_ready || !_mappedPtr) return;

    auto* ctx = GET_SINGLE(Graphics)->GetDeviceContext().Get();
    if (ctx == nullptr) return;

    ctx->Unmap(_ringBuffers[_currentSlot].Get(), 0);
    _mappedPtr = nullptr;
}

void DynamicInstancePool::BindSlice(uint32 elementOffset) const
{
    if (!_ready) return;

    auto* ctx = GET_SINGLE(Graphics)->GetDeviceContext().Get();
    if (ctx == nullptr) return;

    const UINT stride = sizeof(InstancingData);
    const UINT byteOffset = elementOffset * sizeof(InstancingData);

    ctx->IASetVertexBuffers(
        1, 1,
        _ringBuffers[_currentSlot].GetAddressOf(),
        &stride,
        &byteOffset);
}
