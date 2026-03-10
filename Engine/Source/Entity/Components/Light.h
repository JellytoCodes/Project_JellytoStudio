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

private :

};
