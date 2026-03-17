#pragma once
#include "Entity/Components/MonoBehaviour.h"

class CubeScript : public MonoBehaviour
{
public:
	CubeScript();
	virtual ~CubeScript();

	virtual void Update() override;
	virtual void OnCollision(std::shared_ptr<Entity>& other) override;

private:
	bool _destroyed = false;
};