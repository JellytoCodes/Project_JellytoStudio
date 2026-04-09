#pragma once

#include "Scene.h"

class SceneManager
{
	DECLARE_SINGLE(SceneManager)

public:
	void Update();
	void Render();

	template<typename T>
	void ChangeScene(std::unique_ptr<T> scene)
	{
		_currentScene = std::move(scene);
		_currentScene->Start();
	}

	Scene* GetCurrentScene() { return _currentScene.get(); }

private:
	std::unique_ptr<Scene> _currentScene = std::make_unique<Scene>();
};
