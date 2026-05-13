#pragma once
#include "Pipeline/InstancingBuffer.h"

class DynamicInstancePool
{
    DECLARE_SINGLE(DynamicInstancePool)

public:
    static constexpr uint32 kRingCount = 3;
    static constexpr uint32 kMaxInstances = 30000;

    void Init();

    void   BeginFrame();
    uint32 Append(const InstancingData* data, uint32 count);
    void   EndFrame();

    void BindSlice(uint32 byteOffset) const;

    bool IsReady() const { return _ready; }

private:
    ComPtr<ID3D11Buffer> _ringBuffers[kRingCount];

    uint32 _frameIndex = 0;
    uint32 _currentSlot = 0;
    uint32 _writeOffset = 0;
    bool   _firstAppend = true;
    bool   _ready = false;
};