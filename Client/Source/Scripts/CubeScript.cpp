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

	// 이미 파괴 예약됐으면 중복 처리 방지
	_destroyed = true;

	// 현재 씬에서 자신의 Entity를 제거
	auto scene = GET_SINGLE(SceneManager)->GetCurrentScene();
	if (scene)
		scene->Remove(GetEntity());
}