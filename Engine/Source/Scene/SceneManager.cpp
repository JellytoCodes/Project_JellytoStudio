#include "Framework.h"
#include "SceneManager.h"

void SceneManager::Update()
{
	if (!_currentScene) return;
	_currentScene->Update();
}

void SceneManager::Render()
{
	if (!_currentScene) return;
	_currentScene->Render();
}
