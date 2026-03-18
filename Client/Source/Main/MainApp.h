#pragma once

#include "Core/Interfaces/IExecute.h"

class Scene;
class Entity;
class ItemWindow;
class DetailWindow;

class MainApp : public IExecute
{
public:
	virtual void Init()   override;
	virtual void Update() override;
	virtual void Render() override;

	// Application이 창 생성 후 주입
	void SetItemWindow(ItemWindow* w)     { _itemWindow   = w; }
	void SetDetailWindow(DetailWindow* w) { _detailWindow = w; }

	// 씬 접근자 (Application이 창에 씬 주입할 때 사용)
	std::shared_ptr<Scene> GetScene() const { return _scene; }

private:
	void CreateCharacter();
	void CreateCube();
	void CreateSkySphere();
	void CreateLightSphere();
	void CreateFloor();

	// 마우스 좌클릭 피킹 처리
	void UpdatePicking();

	// 화면 좌표 → World Ray 생성
	Ray ScreenToRay(int screenX, int screenY);

	std::shared_ptr<Scene>  _scene;
	std::shared_ptr<Entity> _characterEntity;
	std::shared_ptr<Entity> _cubeEntity;

	// 외부 윈도우 참조 (소유권 없음)
	ItemWindow*   _itemWindow   = nullptr;
	DetailWindow* _detailWindow = nullptr;

	// 현재 픽된 Entity
	std::shared_ptr<Entity> _pickedEntity;
};