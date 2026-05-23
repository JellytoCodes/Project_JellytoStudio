#include "Framework.h"
#include "SharedCBufferManager.h"
#include "Graphics/Graphics.h"

void SharedCBufferManager::EnsureInit()
{
    if (_initialized) return;

    const auto& device = GET_SINGLE(Graphics)->GetDevice();

    _globalBuffer = std::make_unique<ConstantBuffer<GlobalDesc>>();
    _globalBuffer->Create(device);

    _lightBuffer = std::make_unique<ConstantBuffer<LightDesc>>();
    _lightBuffer->Create(device);

    _initialized = true;
}

void SharedCBufferManager::SetGlobal(const Matrix& view, const Matrix& projection)
{
    EnsureInit();

    GlobalDesc next;
    next.V    = view;
    next.P    = projection;
    next.VP   = view * projection;
    next.VInv = view.Invert();

    if (::memcmp(&_globalDesc, &next, sizeof(GlobalDesc)) == 0) return;

    _globalDesc = next;
    _globalBuffer->CopyData(GET_SINGLE(Graphics)->GetDeviceContext(), _globalDesc);
}

void SharedCBufferManager::SetLight(const LightDesc& desc)
{
    EnsureInit();

    if (::memcmp(&_lightDesc, &desc, sizeof(LightDesc)) == 0) return;

    _lightDesc = desc;
    _lightBuffer->CopyData(GET_SINGLE(Graphics)->GetDeviceContext(), _lightDesc);
}