#pragma once
#include "Component.h"

class Mesh;
class Material;

class Terrain : public Component
{
	using Super = Component;
public:
	Terrain();
	virtual ~Terrain();

	virtual void Awake() override;
	virtual void Start() override;
	virtual void Update() override;
	virtual void LateUpdate() override;
	virtual void OnDestroy() override;

	void Create(float sizeX, float sizeZ, std::shared_ptr<Material> material);

	float GetSizeX() const	{ return _sizeX; }
	float GetSizeZ() const	{ return _sizeZ; }

private :
	std::shared_ptr<Mesh> _mesh;
	float _sizeX = 0;
	float _sizeZ = 0;
};
