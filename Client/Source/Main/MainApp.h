#pragma once
#include "Core/Interfaces/IExecute.h"

class Scene;
class Entity;
class Actor;
class IsometricCameraController;
class BlockPlacer;

// ── MainApp ───────────────────────────────────────────────────────────────
// 아이소메트릭 콘텐츠 전용 App
// - Tab: 블록 배치 모드 On/Off
// - 좌클릭: 블록 배치  |  우클릭: 블록 제거
// - Ctrl+S: 씬 저장  |  Ctrl+L: 씬 로드
class MainApp : public IExecute
{
public:
    virtual void Init()   override;
    virtual void Update() override;
    virtual void Render() override;

private:
    void InitScene();
    void CreateCamera();
    void CreateBlockPlacer();

    std::shared_ptr<Scene>  _scene;
    std::shared_ptr<Entity> _characterEntity;
    std::shared_ptr<IsometricCameraController> _isoCamCtrl;
    std::shared_ptr<BlockPlacer> _blockPlacer;

    std::vector<std::shared_ptr<Actor>> _actors;
};