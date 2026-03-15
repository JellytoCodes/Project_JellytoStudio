#pragma once

#include "Core/Interfaces/IExecute.h"

class Scene;

class MainApp : public IExecute
{

public :
	virtual void Init() override;
	virtual void Update() override;
	virtual void Render() override;

private :
	void CreateCamera();
	void CreateModelAnimator();
	void CreateSkySphere();
	void CreateLightSphere();
	void CreateFloor();

	std::shared_ptr<Scene> _scene;
};

