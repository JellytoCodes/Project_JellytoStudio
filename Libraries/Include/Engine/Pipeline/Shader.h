#pragma once
#include "ShaderPass.h"
#include "ShaderTechnique.h"
#include "Resource/Material.h"
#include "Pipeline/ConstantBuffer.h"

struct ShaderDesc
{
	ComPtr<ID3DBlob> blob;
	ComPtr<ID3DX11Effect> effect;
};

class Shader
{
public:
	friend struct ShaderPass;

	Shader(const std::wstring& file);
	~Shader();

	std::wstring GetFile() { return _file; }
	ComPtr<ID3DX11Effect> Effect() { return _shaderDesc.effect; }

	void Draw(UINT technique, UINT pass, UINT vertexCount, UINT startVertexLocation = 0);
	void DrawIndexed(UINT technique, UINT pass, UINT indexCount, UINT startIndexLocation = 0, INT baseVertexLocation = 0);
	void DrawInstanced(UINT technique, UINT pass, UINT vertexCountPerInstance, UINT instanceCount, UINT startVertexLocation = 0, UINT startInstanceLocation = 0);
	void DrawIndexedInstanced(UINT technique, UINT pass, UINT indexCountPerInstance, UINT instanceCount, UINT startIndexLocation = 0, INT baseVertexLocation = 0, UINT startInstanceLocation = 0);

	void Dispatch(UINT technique, UINT pass, UINT x, UINT y, UINT z);

	ComPtr<ID3DX11EffectVariable>						GetVariable(const std::string& name);
	ComPtr<ID3DX11EffectScalarVariable>					GetScalar(const std::string& name);
	ComPtr<ID3DX11EffectVectorVariable>					GetVector(const std::string& name);
	ComPtr<ID3DX11EffectMatrixVariable>					GetMatrix(const std::string& name);
	ComPtr<ID3DX11EffectStringVariable>					GetString(const std::string& name);
	ComPtr<ID3DX11EffectShaderResourceVariable>			GetSRV(const std::string& name);
	ComPtr<ID3DX11EffectRenderTargetViewVariable>		GetRTV(const std::string& name);
	ComPtr<ID3DX11EffectDepthStencilViewVariable>		GetDSV(const std::string& name);
	ComPtr<ID3DX11EffectUnorderedAccessViewVariable>	GetUAV(const std::string& name);
	ComPtr<ID3DX11EffectConstantBuffer>					GetConstantBuffer(const std::string& name);
	ComPtr<ID3DX11EffectShaderVariable>					GetShader(const std::string& name);
	ComPtr<ID3DX11EffectBlendVariable>					GetBlend(const std::string& name);
	ComPtr<ID3DX11EffectDepthStencilVariable>			GetDepthStencil(const std::string& name);
	ComPtr<ID3DX11EffectRasterizerVariable>				GetRasterizer(const std::string& name);
	ComPtr<ID3DX11EffectSamplerVariable>				GetSampler(const std::string& name);

	void PushGlobalData(const Matrix& view, const Matrix& projection);
	void PushTransformData(const TransformDesc& desc);
	void PushLightData(const LightDesc& desc);
	void PushMaterialData(const MaterialDesc& desc);
	void PushBoneData(const BoneDesc& desc);
	void PushKeyframeData(const KeyframeDesc& desc);
	void PushTweenData(const InstancedTweenDesc& desc);

private:
	void CreateEffect();
	ComPtr<ID3D11InputLayout> CreateInputLayout(ComPtr<ID3DBlob> fxBlob, D3DX11_EFFECT_SHADER_DESC* effectVsDesc, std::vector<D3D11_SIGNATURE_PARAMETER_DESC>& params);

	std::wstring _file;
	ShaderDesc _shaderDesc;
	D3DX11_EFFECT_DESC _effectDesc;
	std::shared_ptr<StateBlock> _initialStateBlock;
	std::vector<Technique> _techniques;

	GlobalDesc												_globalDesc;
	std::shared_ptr<ConstantBuffer<GlobalDesc>>				_globalBuffer;
	ComPtr<ID3DX11EffectConstantBuffer>						_globalEffectBuffer;

	TransformDesc											_transformDesc;
	std::shared_ptr<ConstantBuffer<TransformDesc>>			_transformBuffer;
	ComPtr<ID3DX11EffectConstantBuffer>						_transformEffectBuffer;

	LightDesc												_lightDesc;
	std::shared_ptr<ConstantBuffer<LightDesc>>				_lightBuffer;
	ComPtr<ID3DX11EffectConstantBuffer>						_lightEffectBuffer;

	MaterialDesc											_materialDesc;
	std::shared_ptr<ConstantBuffer<MaterialDesc>>			_materialBuffer;
	ComPtr<ID3DX11EffectConstantBuffer>						_materialEffectBuffer;

	BoneDesc												_boneDesc;
	std::shared_ptr<ConstantBuffer<BoneDesc>>				_boneBuffer;
	ComPtr<ID3DX11EffectConstantBuffer>						_boneEffectBuffer;

	KeyframeDesc											_keyframeDesc;
	std::shared_ptr<ConstantBuffer<KeyframeDesc>>			_keyframeBuffer;
	ComPtr<ID3DX11EffectConstantBuffer>						_keyframeEffectBuffer;

	InstancedTweenDesc										_tweenDesc;
	std::shared_ptr<ConstantBuffer<InstancedTweenDesc>>		_tweenBuffer;
	ComPtr<ID3DX11EffectConstantBuffer>						_tweenEffectBuffer;
};

class ShaderManager
{
public:
	static ShaderDesc GetEffect(const std::wstring& fileName);

private:
	static std::unordered_map<std::wstring, ShaderDesc> shaders;
};