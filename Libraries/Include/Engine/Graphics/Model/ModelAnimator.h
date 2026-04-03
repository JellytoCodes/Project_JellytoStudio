#pragma once
#include "Entity/Components/Component.h"
#include "Pipeline/InstancingBuffer.h"

class Shader;
class Model;

struct AnimTransform
{
	using TransformArrayType = std::array<Matrix, MAX_MODEL_TRANSFORMS>;
	std::array<TransformArrayType, MAX_MODEL_KEYFRAMES> transforms;
};

class ModelAnimator : public Component
{
	using Super = Component;

public:
	ModelAnimator(std::shared_ptr<Shader> shader);
	virtual ~ModelAnimator();

	void SetModel(std::shared_ptr<Model> model);
	void SetPass(uint8 pass) { _pass = pass; }
	std::shared_ptr<Shader> GetShader() { return _shader; }
	std::shared_ptr<Model>  GetModel()  { return _model;  }

	virtual void Update() override;
	void UpdateTweenData();

	void RenderInstancing(std::shared_ptr<InstancingBuffer>& buffer);
	InstanceID GetInstanceID();
	TweenDesc& GetTweenDesc() { return _tweenDesc; }

private:
	void PressedKeyForCheckFrame();
	void CreateTexture();
	void CreateAnimationTransform(uint32 index);

	std::vector<AnimTransform>			_animTransforms;
	ComPtr<ID3D11Texture2D>				_texture;
	ComPtr<ID3D11ShaderResourceView>	_srv;

	std::shared_ptr<Shader>				_shader;
	uint8								_pass = 0;
	std::shared_ptr<Model>				_model;

	TweenDesc							_tweenDesc;
};