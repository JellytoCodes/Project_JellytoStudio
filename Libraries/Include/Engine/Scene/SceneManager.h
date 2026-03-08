#pragma once

#include "Scene.h"

class SceneManager
{
	DECLARE_SINGLE(SceneManager)

public :
	void Update();
	void Render();

	template<typename T>
	void ChangeScene(std::shared_ptr<T> scene)
	{
		_currentScene = scene;
		scene->Start();
	}

	std::shared_ptr<Scene> GetCurrentScene() { return _currentScene; }

private :
	std::shared_ptr<Scene> _currentScene = std::make_shared<Scene>();
};
