#pragma once

#include "Core/Interfaces/IExecute.h"

class Scene;
class Entity;
class Actor;
class ItemWindow;
class DetailWindow;
struct DetailInfo;

class MainApp : public IExecute
{
public:
	virtual void Init()   override;
	virtual void Update() override;
	virtual void Render() override;

	void SetItemWindow(ItemWindow* w) { _itemWindow = w; }
	void SetDetailWindow(DetailWindow* w) { _detailWindow = w; }

	std::shared_ptr<Scene> GetScene() const { return _scene; }

private:
	void RegisterActors();      // ItemWindow에 Actor 팩토리 등록
	void SpawnDefaultActors();  // 기본 Actor 스폰
	void CreateCamera();

	void UpdatePicking();
	Ray  ScreenToRay(int screenX, int screenY);
	void FillDetailInfo(std::shared_ptr<Entity> entity, DetailInfo& info);

	std::shared_ptr<Scene>              _scene;
	std::vector<std::shared_ptr<Actor>> _defaultActors;

	ItemWindow* _itemWindow = nullptr;
	DetailWindow* _detailWindow = nullptr;

	std::shared_ptr<Entity> _pickedEntity;
};