#include "Framework.h"
#include "DynamicInstancePool.h"
#include "Graphics/Graphics.h"

void DynamicInstancePool::Init()
{
    auto* device = GET_SINGLE(Graphics)->GetDevice().Get();

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
    _firstAppend = true;
    _ready = true;
}

void DynamicInstancePool::BeginFrame()
{
    _currentSlot = _frameIndex % kRingCount;
    ++_frameIndex;
    _writeOffset = 0;
    _firstAppend = true;
}

uint32 DynamicInstancePool::Append(const InstancingData* data, uint32 count)
{
    if (!_ready || count == 0) return 0;

    const uint32 required = _writeOffset + count;
    if (required > kMaxInstances)
    {
        assert(false && "DynamicInstancePool overflow — kMaxInstances 를 늘리십시오.");
        return _writeOffset;
    }

    auto* ctx = GET_SINGLE(Graphics)->GetDeviceContext().Get();

    const D3D11_MAP mapType = _firstAppend
        ? D3D11_MAP_WRITE_DISCARD
        : D3D11_MAP_WRITE_NO_OVERWRITE;

    D3D11_MAPPED_SUBRESOURCE mapped = {};
    CHECK(ctx->Map(_ringBuffers[_currentSlot].Get(), 0, mapType, 0, &mapped));

    const uint32 byteOffset = _writeOffset * sizeof(InstancingData);
    ::memcpy(static_cast<uint8_t*>(mapped.pData) + byteOffset,
        data,
        sizeof(InstancingData) * count);

    ctx->Unmap(_ringBuffers[_currentSlot].Get(), 0);

    const uint32 returnedOffset = _writeOffset;
    _writeOffset += count;
    _firstAppend = false;

    return returnedOffset;
}

void DynamicInstancePool::EndFrame()
{
    // 현재는 Map/Unmap을 Append 내부에서 완결하므로 별도 Unmap 불필요.
    // 향후 persistent mapping 도입 시 이 함수에서 Unmap.
}

void DynamicInstancePool::BindSlice(uint32 elementOffset) const
{
    if (!_ready) return;

    auto* ctx = GET_SINGLE(Graphics)->GetDeviceContext().Get();

    const UINT stride = sizeof(InstancingData);
    const UINT byteOffset = elementOffset * sizeof(InstancingData);

    ctx->IASetVertexBuffers(
        1, 1,
        _ringBuffers[_currentSlot].GetAddressOf(),
        &stride,
        &byteOffset);
}