#pragma once
#include "Entity/Components/Component.h"

class Light : Component
{
	using Super = Component;
public:
	Light();
	virtual ~Light();

	virtual void Awake() override;
	virtual void Start() override;
	virtual void Update() override;
	virtual void LateUpdate() override;
	virtual void OnDestroy() override;

	LightDesc& GetLightDesc()					{ return _desc; }

	void SetLightDesc(const LightDesc& desc)	{ _desc = desc; }
	void SetAmbient(const Color& color)			{ _desc.ambient = color; }
	void SetDiffuse(const Color& color)			{ _desc.diffuse = color; }
	void SetSpecular(const Color& color)		{ _desc.specular = color; }
	void SetEmissive(const Color& color)		{ _desc.emissive = color; }
	void SetLightDirection(Vec3 direction)		{ _desc.direction = direction; }

private :
	LightDesc _desc;
};
