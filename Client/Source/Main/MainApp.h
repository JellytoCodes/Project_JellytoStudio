#pragma once
#include "Core/Interfaces/IExecute.h"

class Scene;
class Entity;
class Actor;
class IsometricCameraController;

// ── MainApp ───────────────────────────────────────────────────────────────
// 아이소메트릭 게임 콘텐츠 전용 App
// - 에디터 윈도우 없음 (ToolWindow / DetailWindow 없음)
// - 순수 게임 루프: 씬 초기화 → 캐릭터/맵/카메라 → 게임 로직
class MainApp : public IExecute
{
public:
    virtual void Init()   override;
    virtual void Update() override;
    virtual void Render() override;

private:
    void InitScene();
    void CreateCamera();

    std::shared_ptr<Scene>  _scene;
    std::shared_ptr<Entity> _characterEntity;
    std::shared_ptr<IsometricCameraController> _isoCamCtrl;

    std::vector<std::shared_ptr<Actor>> _actors;
};