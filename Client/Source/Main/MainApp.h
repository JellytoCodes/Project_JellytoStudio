#pragma once

#include "Core/Interfaces/IExecute.h"

class Scene;
class Entity;
class Actor;
class IsometricCameraController;

class MainApp : public IExecute
{
public:
	virtual void Init()   override;
	virtual void Update() override;
	virtual void Render() override;

private:
	void RegisterActors();      // ItemWindow에 Actor 팩토리 등록
	void SpawnDefaultActors();  // 기본 Actor 스폰
	void CreateCamera();

	void UpdatePicking();
	void FillDetailInfo(std::shared_ptr<Entity> entity, DetailInfo& info);

	std::vector<std::shared_ptr<Actor>> _defaultActors;

	std::shared_ptr<Entity> _pickedEntity;
	std::shared_ptr<Entity> _characterEntity;
	std::shared_ptr<IsometricCameraController> _isoCamCtrl;
};