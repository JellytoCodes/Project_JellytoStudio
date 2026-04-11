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
	EditorApp();
	~EditorApp();

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

	std::unique_ptr<Scene> _scene;

	ItemWindow*   _itemWindow   = nullptr;
	DetailWindow* _detailWindow = nullptr;

	std::vector<std::unique_ptr<Actor>> _defaultActors;

	Entity*                    _pickedEntity    = nullptr;
	Entity*                    _characterEntity = nullptr;
	IsometricCameraController* _isoCamCtrl      = nullptr;
};
