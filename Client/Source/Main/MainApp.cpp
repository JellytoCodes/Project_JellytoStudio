#include "pch.h"
#include "MainApp.h"

#include "Actors.h"

#include "Resource/Managers/ResourceManager.h"
#include "Core/Managers/InputManager.h"
#include "Core/Managers/TimeManager.h"
#include "Scene/SceneManager.h"
#include "Scene/Scene.h"
#include "Entity/Entity.h"
#include "Entity/Actor.h"
#include "Entity/Components/Transform.h"
#include "Entity/Components/Camera.h"
#include "Entity/Managers/CollisionManager.h"
#include "Graphics/Model/ModelAnimator.h"
#include "Graphics/Model/Model.h"
#include "Graphics/Model/ModelAnimation.h"
#include "Scripts/IsometricCameraController.h"
#include "App/Managers/WindowManager.h"

// ── Init ─────────────────────────────────────────────────────────────────

void MainApp::Init()
{
	GET_SINGLE(ResourceManager)->Init();

    _scene = std::make_shared<Scene>();
    _scene->SetName(L"Main Scene");

    _itemWindow = GET_SINGLE(WindowManager)->GetWindow<ItemWindow>(L"ItemWindow");
    _detailWindow = GET_SINGLE(WindowManager)->GetWindow<DetailWindow>(L"DetailWindow");

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
    spawn(std::make_shared<CubeActor>());
    spawn(std::make_shared<SphereActor>());

    auto charActor = std::make_shared<CharacterActor>();
    charActor->Spawn(_scene);
    _defaultActors.push_back(charActor);
    _characterEntity = charActor->GetEntity();
}

void MainApp::CreateCamera()
{
    auto cam = std::make_shared<Entity>(L"카메라");
    cam->AddComponent(std::make_shared<Transform>());
    cam->AddComponent(std::make_shared<Camera>());

    auto isoCtrl = std::make_shared<IsometricCameraController>();
    isoCtrl->SetDistance(20.f);
    isoCtrl->SetPanSpeed(10.f);
    isoCtrl->SetZoomSpeed(15.f);
    isoCtrl->SetMinDistance(5.f);
    isoCtrl->SetMaxDistance(60.f);
    cam->AddComponent(isoCtrl);
    _isoCamCtrl = isoCtrl;

    _scene->Add(cam);
    _scene->SetMainCamera(cam->GetComponent<Camera>());

    if (_characterEntity)
        isoCtrl->SetTarget(_characterEntity);
}

// ── Update ───────────────────────────────────────────────────────────────

void MainApp::Update()
{
    CollisionManager::CheckCollision(_scene);
    UpdatePicking();

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
    // ── Delete: 선택된 Entity 제거 ─────────────────────────────────────
    if (GET_SINGLE(InputManager)->GetButtonDown(KEY_TYPE::DEL) || GET_SINGLE(InputManager)->GetButtonDown(KEY_TYPE::P))
    {
        auto selected = _detailWindow ? _detailWindow->GetSelectedEntity() : nullptr;
        if (selected && _scene)
        {
            wchar_t dbgDel[256];
            swprintf_s(dbgDel, L"[Delete] 씬에서 제거: %s\n", selected->GetEntityName().c_str());
            ::OutputDebugStringW(dbgDel);

            _scene->Remove(selected);
            _pickedEntity = nullptr;
            if (_detailWindow)
            {
                _detailWindow->ClearDetail();
                _detailWindow->RefreshEntityList();
            }
        }
        else
        {
            ::OutputDebugStringW(L"[Delete] 선택된 Entity 없음\n");
        }
    }

    // ── 좌클릭 피킹 ────────────────────────────────────────────────────
    if (!GET_SINGLE(InputManager)->GetButtonDown(KEY_TYPE::LBUTTON)) return;

    POINT mp = GET_SINGLE(InputManager)->GetMousePos();

    wchar_t dbgClick[128];
    swprintf_s(dbgClick, L"[Picking] 좌클릭 감지 - 스크린 좌표: (%d, %d)\n", mp.x, mp.y);
    ::OutputDebugStringW(dbgClick);

    auto picked = _scene->Pick((int32)mp.x, (int32)mp.y);
    _pickedEntity = picked;

    if (!picked)
    {
        ::OutputDebugStringW(L"[Picking] 히트 없음\n");
        if (_detailWindow) _detailWindow->ClearDetail();
        return;
    }

    wchar_t dbgHit[256];
    swprintf_s(dbgHit, L"[Picking] 히트! Entity: %s\n", picked->GetEntityName().c_str());
    ::OutputDebugStringW(dbgHit);

    if (!_detailWindow) return;
    DetailInfo info;
    FillDetailInfo(picked, info);
    _detailWindow->UpdateDetail(info);
    _detailWindow->SelectEntity(picked);
    if (!_detailWindow->IsVisible()) _detailWindow->Show();
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