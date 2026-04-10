#pragma once
#include "Core/Interfaces/IExecute.h"

class Scene;
class Entity;
class Actor;
class IsometricCameraController;
class BlockPlacer;
class PaletteWidget;

class MainApp : public IExecute
{
public:
	MainApp();
	~MainApp();

	virtual void Init()   override;
	virtual void Update() override;
	virtual void Render() override;

private:
	void InitScene();
	void CreateCamera();
	void CreatePlacementSystem();

	std::unique_ptr<Scene> _scene;

	Entity*                    _characterEntity = nullptr;
	Entity*                    _startBlock      = nullptr;
	IsometricCameraController* _isoCamCtrl      = nullptr;
	BlockPlacer*               _blockPlacer     = nullptr;
	PaletteWidget*             _palette         = nullptr;

	std::vector<std::unique_ptr<Actor>> _actors;
};
