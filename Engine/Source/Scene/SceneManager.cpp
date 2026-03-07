
#include "Framework.h"
#include "SceneManager.h"

void SceneManager::Update()
{
	_currentScene->Update();
}

void SceneManager::Render()
{
	_currentScene->Render();
}
