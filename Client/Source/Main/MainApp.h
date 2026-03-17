#pragma once

#include "Core/Interfaces/IExecute.h"

class Scene;
class Entity;

class MainApp : public IExecute
{
public:
	virtual void Init() override;
	virtual void Update() override;
	virtual void Render() override;

private:
	void CreateCharacter();
	void CreateCube();
	void CreateSkySphere();
	void CreateLightSphere();
	void CreateFloor();

	std::shared_ptr<Scene>  _scene;
	std::shared_ptr<Entity> _characterEntity;
	std::shared_ptr<Entity> _cubeEntity;
};