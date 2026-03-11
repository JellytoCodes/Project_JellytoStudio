#pragma once

#include "Entity/Components/Component.h"
#include "Pipeline/ConstantBuffer.h"

class Model;
class Shader;
class Material;
class InstancingBuffer;

class ModelRenderer : public Component
{
	using Super = Component;

public:
	ModelRenderer(std::shared_ptr<Shader> shader);
	virtual ~ModelRenderer();

	void Awake() override;
	void Start() override;

	void SetModel(std::shared_ptr<Model> model);
	void SetPass(uint8 pass) { _pass = pass; }

	void RenderInstancing(std::shared_ptr<InstancingBuffer>& buffer);
	InstanceID GetInstanceID();

private:
	std::shared_ptr<ConstantBuffer<TransformData>>	_constantBuffer;

	std::shared_ptr<Shader>							_shader;
	uint8											_pass = 0;
	std::shared_ptr<Model>							_model;
};