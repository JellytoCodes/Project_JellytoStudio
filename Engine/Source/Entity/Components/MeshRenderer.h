#pragma once
#include "Component.h"

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

	void SetMesh(const shared_ptr<Mesh>& mesh)					{ _mesh = mesh; }
	void SetMaterial(const shared_ptr<Material>& material)		{ _material = material; }
	void SetPass(uint8 pass)									{ _pass = pass; }

private:
	shared_ptr<Mesh> _mesh;
	shared_ptr<Material> _material;
	uint8 _pass = 0;
};
