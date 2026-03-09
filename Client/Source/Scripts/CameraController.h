#pragma once

#include "Entity/Components/MonoBehaviour.h"

class CameraController : public MonoBehaviour
{
public :
	CameraController();
	virtual ~CameraController();

	virtual void Awake() override;
	virtual void Start() override;
	virtual void Update() override;
	virtual void LateUpdate() override;
	virtual void OnDestroy() override;

private :
	float _speed = 5.f;
	float _rotationSpeed = .1f;
};