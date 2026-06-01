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

    void BindSlice(uint32 elementOffset) const;

    bool IsReady() const { return _ready; }

    uint32 GetUsedInstances() const { return _writeOffset; }

    uint32 GetCurrentSlot() const { return _currentSlot; }

private:
    ComPtr<ID3D11Buffer> _ringBuffers[kRingCount];

    uint8_t* _mappedPtr = nullptr;

    uint32 _frameIndex = 0;
    uint32 _currentSlot = 0;
    uint32 _writeOffset = 0;
    bool   _ready = false;
};
