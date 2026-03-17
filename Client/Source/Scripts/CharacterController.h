#pragma once
#include "Entity/Components/MonoBehaviour.h"

class CharacterController : public MonoBehaviour
{
public:
	CharacterController();
	virtual ~CharacterController();

	virtual void Awake() override;
	virtual void Start() override;
	virtual void Update() override;
	virtual void LateUpdate() override;
	virtual void OnDestroy() override;

	void SetMoveSpeed(float speed) { _moveSpeed = speed; }
	void SetRotationSpeed(float speed) { _rotationSpeed = speed; }

private:
	float _moveSpeed = 3.f;
	float _rotationSpeed = 90.f;	// 초당 회전 각도 (degree)
};