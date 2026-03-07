#pragma once
#include "Component.h"

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

};

