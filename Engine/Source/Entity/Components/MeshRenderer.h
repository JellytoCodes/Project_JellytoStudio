#pragma once

#include "Component.h"
#include "Pipeline/ConstantBuffer.h"

class Shader;
class Material;
class Mesh;

class MeshRenderer : public Component
{
	using Super = Component;

public :
	MeshRenderer();
	virtual ~MeshRenderer();

	virtual void Awake() override;
	virtual void Start() override;
	virtual void Update() override;
	virtual void LateUpdate() override;
	virtual void OnDestroy() override;

	virtual void Render() override;

	void SetMesh(const shared_ptr<Mesh>& mesh)					{ _mesh = mesh; }
	void SetMaterial(const shared_ptr<Material>& material)		{ _material = material; }
	void SetPass(uint8 pass)									{ _pass = pass; }

private:
	std::shared_ptr<Mesh> _mesh;
	std::shared_ptr<Shader> _shader;
	std::shared_ptr<Material> _material;

	uint8 _pass = 0;

	std::shared_ptr<ConstantBuffer<TransformData>> _constantBuffer;

	Matrix _matWorld = Matrix::Identity;
};