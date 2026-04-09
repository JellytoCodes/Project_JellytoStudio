#pragma once

#include "Entity/Components/Component.h"
#include "Pipeline/ConstantBuffer.h"
#include "Pipeline/InstancingBuffer.h"

class Model;
class Shader;
class Material;
class InstancingBuffer;

class ModelRenderer : public Component
{
	using Super = Component;

public:
	ModelRenderer(std::shared_ptr<Shader> shader, bool bIsSkinned = true);
	virtual ~ModelRenderer();

	void Awake() override;
	void Start() override;

	void SetModel(std::shared_ptr<Model> model);
	void SetPass(uint8 pass) { _pass = pass; }

	void SetModelScale(const Vec3& scale)	{ _modelScale = scale; }
	Vec3   GetModelScale()      const		{ return _modelScale; }
	Matrix GetModelScaleMatrix()const		{ return Matrix::CreateScale(_modelScale); }

	void RenderInstancing(InstancingBuffer* buffer);
	InstanceID GetInstanceID();

private:
	std::unique_ptr<ConstantBuffer<TransformData>>	_constantBuffer;

	std::shared_ptr<Shader>							_shader;
	std::shared_ptr<Model>							_model;
	uint8											_pass    = 0;
	
	Vec3											_modelScale = Vec3(1.f, 1.f, 1.f);
	bool											_bIsSkinned;
};
