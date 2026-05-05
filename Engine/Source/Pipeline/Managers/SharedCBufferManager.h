#pragma once
#include "Pipeline/ConstantBuffer.h"
#include "Types/ShaderDesc.h"

class SharedCBufferManager
{
    DECLARE_SINGLE(SharedCBufferManager);

public:
    void SetGlobal(const Matrix& view, const Matrix& projection);
    void SetLight(const LightDesc& desc);

    ID3D11Buffer* GetGlobalBuffer() const { return _globalBuffer->GetComPtr().Get(); }
    ID3D11Buffer* GetLightBuffer()  const { return _lightBuffer->GetComPtr().Get(); }

private:
    void EnsureInit();

    std::unique_ptr<ConstantBuffer<GlobalDesc>> _globalBuffer;
    std::unique_ptr<ConstantBuffer<LightDesc>>  _lightBuffer;

    GlobalDesc _globalDesc = {};
    LightDesc  _lightDesc = {};
    bool       _initialized = false;
};