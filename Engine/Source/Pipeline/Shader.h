#pragma once

class Shader
{
public:
    Shader();
    ~Shader();

    bool Create(const ComPtr<ID3D11Device>& device, const std::wstring& path, const std::string& vsEntry = "VS", const std::string& psEntry = "PS");

    void Bind(const ComPtr<ID3D11DeviceContext>& deviceContext);

private:
    bool CompileShader(const std::wstring& path, const std::string& entry, const std::string& profile, ComPtr<ID3DBlob>& blob);
    void CreateInputLayout(const ComPtr<ID3D11Device>& device, ComPtr<ID3DBlob> vsBlob);

private:
    ComPtr<ID3D11VertexShader> _vertexShader;
    ComPtr<ID3D11PixelShader>  _pixelShader;
    ComPtr<ID3D11InputLayout>  _inputLayout;
};