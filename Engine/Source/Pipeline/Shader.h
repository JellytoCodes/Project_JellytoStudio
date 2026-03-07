#pragma once

struct ShaderDesc
{
	ComPtr<ID3DBlob> blob;
	ComPtr<ID3DX11Effect> effect;
};

class Shader
{
public:
    Shader();
    ~Shader();

    bool Create(const ComPtr<ID3D11Device>& device, const std::wstring& path, const std::string& vsEntry = "VS", const std::string& psEntry = "PS");

    void Bind(const ComPtr<ID3D11DeviceContext>& deviceContext);

	ComPtr<ID3DX11EffectShaderResourceVariable> GetSRV(string name);
	ComPtr<ID3DX11EffectRenderTargetViewVariable> GetRTV(string name);
	ComPtr<ID3DX11EffectDepthStencilViewVariable> GetDSV(string name);

private:
    bool CompileShader(const std::wstring& path, const std::string& entry, const std::string& profile, ComPtr<ID3DBlob>& blob);
    void CreateInputLayout(const ComPtr<ID3D11Device>& device, ComPtr<ID3DBlob> vsBlob);

private:
    ShaderDesc _shaderDesc;

    ComPtr<ID3D11VertexShader> _vertexShader;
    ComPtr<ID3D11PixelShader>  _pixelShader;
    ComPtr<ID3D11InputLayout>  _inputLayout;
};