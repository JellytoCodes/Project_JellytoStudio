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
    void FillDetailInfo(std::shared_ptr<Entity> entity, DetailInfo& info);

    std::shared_ptr<Scene>          _scene;
    std::shared_ptr<ItemWindow>     _itemWindow;
    std::shared_ptr<DetailWindow>   _detailWindow;

    std::vector<std::shared_ptr<Actor>> _defaultActors;

    std::shared_ptr<Entity>                     _pickedEntity;
    std::shared_ptr<Entity>                     _characterEntity;
    std::shared_ptr<IsometricCameraController>  _isoCamCtrl;
};