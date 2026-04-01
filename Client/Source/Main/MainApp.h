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
    virtual void Init()   override;
    virtual void Update() override;
    virtual void Render() override;

private:
    void InitScene();
    void CreateCamera();
    void CreatePlacementSystem();

    std::shared_ptr<Scene>                      _scene;
    std::shared_ptr<Entity>                     _characterEntity;
    std::shared_ptr<Entity>                     _startBlock;
    std::shared_ptr<IsometricCameraController>  _isoCamCtrl;
    std::shared_ptr<BlockPlacer>                _blockPlacer;
    std::shared_ptr<PaletteWidget>              _palette;

    std::vector<std::shared_ptr<Actor>>         _actors;
};