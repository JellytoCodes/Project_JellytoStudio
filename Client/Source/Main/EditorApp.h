#pragma once

#include "Core/Interfaces/IExecute.h"
#include "App/ItemWindow.h"
#include "App/DetailWindow.h"
#include "App/ChunkDebugWindow.h"
#include "UI/BlockTestPanel.h"
#include "UI/StressPanel.h"

class Scene;
class Entity;
class Actor;
class BlockPlacer;
class ItemWindow;
class DetailWindow;
class ChunkDebugWindow;
class BlockTestPanel;
class StressPanel;
class IsometricCameraController;
class UIText;

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
    void FillDetailInfo(Entity* entity, DetailInfo& out);

    void SetSaveStatus(const std::wstring& msg);

    std::unique_ptr<Scene>              _scene;

    ItemWindow*                         _itemWindow       = nullptr;
    DetailWindow*                       _detailWindow     = nullptr;
    ChunkDebugWindow*                   _chunkDebugWindow = nullptr;
    BlockTestPanel*                     _blockTestPanel   = nullptr;
    StressPanel*                        _stressPanel      = nullptr;

    std::vector<std::unique_ptr<Actor>> _defaultActors;

    Entity*                             _pickedEntity     = nullptr;
    Entity*                             _characterEntity  = nullptr;
    IsometricCameraController*          _isoCamCtrl       = nullptr;
    BlockPlacer*                        _stressPlacer     = nullptr;

    UIText*      _saveStatusText  = nullptr;
    std::wstring _saveStatusMsg;
    float        _saveStatusTimer = 0.f;
    static constexpr float kSaveStatusDuration   = 3.0f;

    float _chunkRefreshTimer  = 0.f;
    float _stressRefreshTimer = 0.f;
    static constexpr float kChunkRefreshInterval  = ChunkDebugWindow::kRefreshInterval;
    static constexpr float kStressRefreshInterval = StressPanel::kRefreshInterval;
};