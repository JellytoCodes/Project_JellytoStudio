
#pragma once
#include "Core/Interfaces/IExecute.h"
#include "UI/DebugHUD.h"
#include "UI/InventoryData.h"

class Scene;
class Entity;
class Actor;
class IsometricCameraController;
class BlockPlacer;
class OneBlockScript;
class PaletteWidget;
class InventoryWidget;

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
    void CreateInventorySystem();

    std::unique_ptr<Scene>              _scene;
    std::vector<std::unique_ptr<Actor>> _actors;

    Entity*                    _characterEntity = nullptr;
    Entity*                    _startBlock      = nullptr;
    IsometricCameraController* _isoCamCtrl      = nullptr;
    BlockPlacer*               _blockPlacer     = nullptr;
    OneBlockScript*            _oneBlockScript  = nullptr;
    PaletteWidget*             _palette         = nullptr;

    InventoryData              _inventoryData;             
    InventoryWidget*           _inventoryWidget = nullptr;

    DebugHUD                   _debugHUD;
};