#include "pch.h"
#include "MainApp.h"
#include "Actors.h"

#include "Core/Managers/InputManager.h"
#include "Core/Managers/TimeManager.h"
#include "Resource/Managers/ResourceManager.h"
#include "Scene/Scene.h"
#include "Scene/SceneManager.h"
#include "Entity/Entity.h"
#include "Entity/Actor.h"
#include "Entity/Components/Transform.h"
#include "Entity/Components/Camera.h"
#include "Entity/Managers/CollisionManager.h"
#include "Graphics/Model/ModelAnimator.h"
#include "Graphics/Model/Model.h"
#include "Graphics/Model/ModelAnimation.h"
#include "Scripts/CameraController.h"

// ── Init ─────────────────────────────────────────────────────────────────

void MainApp::Init()
{
    GET_SINGLE(ResourceManager)->Init();

    _scene = std::make_shared<Scene>();
    _scene->SetName(L"Main Scene");

    SpawnDefaultActors();
    CreateCamera();

    GET_SINGLE(SceneManager)->ChangeScene(_scene);

    if (_itemWindow)   _itemWindow->SetScene(_scene);
    if (_detailWindow) _detailWindow->SetScene(_scene);

    RegisterActors();
}

void MainApp::RegisterActors()
{
    if (!_itemWindow) return;

    _itemWindow->RegisterActor(L"SkySphere", [] { return std::make_shared<SkySphereActor>(); });
    _itemWindow->RegisterActor(L"Floor", [] { return std::make_shared<FloorActor>();      });
    _itemWindow->RegisterActor(L"Cube", [] { return std::make_shared<CubeActor>();       });
    _itemWindow->RegisterActor(L"Sphere", [] { return std::make_shared<SphereActor>();       });
    _itemWindow->RegisterActor(L"Character", [] { return std::make_shared<CharacterActor>();  });
}

void MainApp::SpawnDefaultActors()
{
    auto spawn = [&](std::shared_ptr<Actor> actor)
        {
            actor->Spawn(_scene);
            _defaultActors.push_back(actor);
        };

    spawn(std::make_shared<SkySphereActor>());
    spawn(std::make_shared<FloorActor>());
}

void MainApp::CreateCamera()
{
    auto cam = std::make_shared<Entity>(L"카메라");
    cam->AddComponent(std::make_shared<Transform>());
    cam->AddComponent(std::make_shared<Camera>());
    cam->AddComponent(std::make_shared<CameraController>());
    cam->GetTransform()->SetLocalPosition(Vec3(0.f, 1.5f, -3.f));
    _scene->Add(cam);
    _scene->SetMainCamera(cam->GetComponent<Camera>());
}

// ── Update ───────────────────────────────────────────────────────────────

void MainApp::Update()
{
    CollisionManager::CheckCollision(_scene);
    UpdatePicking();

    // DetailWindow 오브젝트 목록 0.5초마다 갱신
    if (_detailWindow && _detailWindow->IsVisible())
    {
        static float timer = 0.f;
        timer += GET_SINGLE(TimeManager)->GetDeltaTime();
        if (timer >= 0.5f) { timer = 0.f; _detailWindow->RefreshEntityList(); }
    }
}

void MainApp::Render() {}

// ── 피킹 ─────────────────────────────────────────────────────────────────

void MainApp::UpdatePicking()
{
    if (!GET_SINGLE(InputManager)->GetButtonDown(KEY_TYPE::LBUTTON)) return;

    POINT mp = GET_SINGLE(InputManager)->GetMousePos();
    Ray   ray = ScreenToRay(mp.x, mp.y);

    auto picked = _scene->Pick(ray);
    if (picked == _pickedEntity) return;
    _pickedEntity = picked;

    if (!_detailWindow) return;

    if (!picked) { _detailWindow->ClearDetail(); return; }

    DetailInfo info;
    FillDetailInfo(picked, info);
    _detailWindow->UpdateDetail(info);
    _detailWindow->SelectEntity(picked); // 목록 강조

    if (!_detailWindow->IsVisible())
        _detailWindow->Show();
}

void MainApp::FillDetailInfo(std::shared_ptr<Entity> entity, DetailInfo& info)
{
    info.entityLabel = L"Entity";

    auto tf = entity->GetTransform();
    if (tf)
    {
        Vec3 pos = tf->GetPosition(), rot = tf->GetRotation(), scl = tf->GetScale();
        info.tx = pos.x; info.ty = pos.y; info.tz = pos.z;
        info.rx = XMConvertToDegrees(rot.x);
        info.ry = XMConvertToDegrees(rot.y);
        info.rz = XMConvertToDegrees(rot.z);
        info.sx = scl.x; info.sy = scl.y; info.sz = scl.z;
    }

    auto animator = entity->GetComponent<ModelAnimator>();
    if (animator && animator->GetModel())
    {
        auto mdl = animator->GetModel();
        info.entityLabel = info.modelName =
            (mdl->GetMeshCount() > 0) ? mdl->GetMeshByIndex(0)->name : L"Unknown";
        info.boneCount = (int)mdl->GetBoneCount();
        info.meshCount = (int)mdl->GetMeshCount();

        if (mdl->GetAnimationCount() > 0)
        {
            auto anim = mdl->GetAnimationByIndex(0);
            info.animName = anim->name;
            info.frameCount = (int)anim->frameCount;
            info.frameRate = anim->frameRate;
            info.duration = anim->duration;
        }
    }
}

Ray MainApp::ScreenToRay(int screenX, int screenY)
{
    auto scene = GET_SINGLE(SceneManager)->GetCurrentScene();
    auto camera = scene ? scene->GetMainCamera() : nullptr;
    if (!camera) return Ray(Vec3::Zero, Vec3::Forward);

    float W = camera->GetWidth(), H = camera->GetHeight();
    float ndcX = (2.f * screenX / W) - 1.f;
    float ndcY = 1.f - (2.f * screenY / H);

    Matrix invProj = camera->GetProjectionMatrix().Invert();
    Matrix invView = camera->GetViewMatrix().Invert();

    Vec4 viewDir = Vec4::Transform(Vec4(ndcX, ndcY, 1.f, 0.f), invProj);
    viewDir.w = 0.f;
    Vec4 worldDir = Vec4::Transform(viewDir, invView);
    Vec3 dir(worldDir.x, worldDir.y, worldDir.z);
    dir.Normalize();

    Vec3 origin(invView._41, invView._42, invView._43);
    return Ray(origin, dir);
}