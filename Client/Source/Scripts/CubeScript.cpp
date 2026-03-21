
#include "pch.h"
#include "CubeScript.h"
#include "Scene/SceneManager.h"
#include "Scene/Scene.h"
#include "Entity/Entity.h"

CubeScript::CubeScript()
{

}

CubeScript::~CubeScript()
{

}

void CubeScript::Update()
{

}

void CubeScript::OnCollision(std::shared_ptr<Entity>& other)
{
	if (_destroyed) return;

	_destroyed = true;

	auto scene = GET_SINGLE(SceneManager)->GetCurrentScene();
	if (scene)
		scene->Remove(GetEntity());
}