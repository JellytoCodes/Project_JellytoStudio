
#include "Framework.h"
#include "Shader.h"

Shader::Shader() {}
Shader::~Shader() {}

bool Shader::Create(const ComPtr<ID3D11Device>& device, const std::wstring& path, const std::string& vsEntry, const std::string& psEntry)
{
    ComPtr<ID3DBlob> vsBlob;
    ComPtr<ID3DBlob> psBlob;

    // 1. Vertex Shader
    if (CompileShader(path, vsEntry, "vs_5_0", vsBlob))
    {
        HRESULT hr = device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &_vertexShader);
        if (FAILED(hr)) return false;

        CreateInputLayout(device, vsBlob);
    }

    // 2. Pixel Shader
    if (CompileShader(path, psEntry, "ps_5_0", psBlob))
    {
        HRESULT hr = device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &_pixelShader);
        if (FAILED(hr)) return false;
    }

    return (_vertexShader && _pixelShader);
}

bool Shader::CompileShader(const std::wstring& path, const std::string& entry, const std::string& profile, ComPtr<ID3DBlob>& blob)
{
    UINT flags = D3DCOMPILE_ENABLE_STRICTNESS;

#ifdef _DEBUG
    flags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

    ComPtr<ID3DBlob> errorBlob;
    HRESULT hr = D3DCompileFromFile(
        path.c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
        entry.c_str(), profile.c_str(), flags, 0, &blob, &errorBlob
    );

    if (FAILED(hr))
    {
        if (errorBlob)
        {
            OutputDebugStringA((char*)errorBlob->GetBufferPointer());
        }
        return false;
    }
    return true;
}

void Shader::CreateInputLayout(const ComPtr<ID3D11Device>& device, ComPtr<ID3DBlob> vsBlob)
{
    D3D11_INPUT_ELEMENT_DESC layout[] = 
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 }
    };

    device->CreateInputLayout(layout, ARRAYSIZE(layout), vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &_inputLayout);
}

void Shader::Bind(const ComPtr<ID3D11DeviceContext>& deviceContext)
{
    deviceContext->IASetInputLayout(_inputLayout.Get());
    deviceContext->VSSetShader(_vertexShader.Get(), nullptr, 0);
    deviceContext->PSSetShader(_pixelShader.Get(), nullptr, 0);
}