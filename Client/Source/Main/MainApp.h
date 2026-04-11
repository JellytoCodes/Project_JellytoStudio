
#pragma once
#include "Core/Interfaces/IExecute.h"
#include "UI/InventoryData.h"   // ★ POD 데이터, 직접 소유

class Scene;
class Entity;
class Actor;
class IsometricCameraController;
class BlockPlacer;
class PaletteWidget;
class InventoryWidget;          // ★ 순방향 선언

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

    Entity*                     _characterEntity = nullptr;
    Entity*                     _startBlock      = nullptr;
    IsometricCameraController*  _isoCamCtrl      = nullptr;
    BlockPlacer*                _blockPlacer     = nullptr;
    PaletteWidget*              _palette         = nullptr;

    InventoryData               _inventoryData;
    InventoryWidget*            _inventoryWidget = nullptr;
};