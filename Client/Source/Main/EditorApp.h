#pragma once

#include "Core/Interfaces/IExecute.h"
#include "App/ItemWindow.h"
#include "App/DetailWindow.h"
#include "App/ChunkDebugWindow.h"
#include "UI/BlockTestPanel.h"
#include "UI/StressPanel.h"
#include "UI/PickDebugPanel.h"

class Scene;
class Entity;
class Actor;
class BlockPlacer;
class Light;
class ItemWindow;
class DetailWindow;
class ChunkDebugWindow;
class BlockTestPanel;
class StressPanel;
class PickDebugPanel;
class FreeCameraController;

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

    void UpdatePicking();
    void FillDetailInfo(Entity* entity, DetailInfo& out);

    void SetSaveStatus(const std::wstring& msg);

    std::unique_ptr<Scene>              _scene;

    ItemWindow*                         _itemWindow       = nullptr;
    DetailWindow*                       _detailWindow     = nullptr;
    ChunkDebugWindow*                   _chunkDebugWindow = nullptr;
    BlockTestPanel*                     _blockTestPanel   = nullptr;
    StressPanel*                        _stressPanel      = nullptr;
    PickDebugPanel*                     _pickDebugPanel   = nullptr;

    std::vector<std::unique_ptr<Actor>> _defaultActors;
    std::unique_ptr<Light>              _captureLight;

    Entity*                             _pickedEntity     = nullptr;
    FreeCameraController*               _freeCamCtrl      = nullptr;
    BlockPlacer*                        _stressPlacer     = nullptr;

    float _chunkRefreshTimer  = 0.f;
    float _stressRefreshTimer = 0.f;
    float _pickDebugRefreshTimer = 0.f;
    static constexpr float kChunkRefreshInterval  = ChunkDebugWindow::kRefreshInterval;
    static constexpr float kStressRefreshInterval = StressPanel::kRefreshInterval;
    static constexpr float kPickDebugRefreshInterval = PickDebugPanel::kRefreshInterval;
};
