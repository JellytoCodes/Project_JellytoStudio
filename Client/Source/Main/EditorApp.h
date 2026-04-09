#pragma once

#include "Core/Interfaces/IExecute.h"
#include "App/ItemWindow.h"
#include "App/DetailWindow.h"

class Scene;
class Entity;
class Actor;
class ItemWindow;
class DetailWindow;
class IsometricCameraController;

class EditorApp : public IExecute
{
public:
	virtual void Init()   override;
	virtual void Update() override;
	virtual void Render() override;

private:
	void RegisterActors();
	void SpawnDefaultActors();
	void CreateCamera();
	void CreateHUD();

	void UpdatePicking();
	void FillDetailInfo(Entity* entity, DetailInfo& info);

	// ── 이전: shared_ptr<Scene> — SceneManager와 이중 소유 ─────────────
	// 변경: unique_ptr<Scene> — Init()에서 ChangeScene()으로 소유권 이전
	std::unique_ptr<Scene> _scene;

	// Win32 윈도우: WindowManager가 소유, EditorApp은 관찰자
	ItemWindow*   _itemWindow   = nullptr;
	DetailWindow* _detailWindow = nullptr;

	// Actor 컨테이너: Actor 객체 소유
	std::vector<std::unique_ptr<Actor>> _defaultActors;

	// Entity*: Scene이 소유, EditorApp은 관찰자
	Entity*                    _pickedEntity    = nullptr;
	Entity*                    _characterEntity = nullptr;
	IsometricCameraController* _isoCamCtrl      = nullptr;
};
