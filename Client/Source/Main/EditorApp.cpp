#include "pch.h"
#include "EditorApp.h"

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
#include "Entity/Components/Light.h"
#include "App/Managers/WindowManager.h"
#include "UI/Widget.h"
#include "UI/Components/UIText.h"
#include "UI/Components/UIButton.h"
#include "Scene/SceneSerializer.h"

// ── Init ─────────────────────────────────────────────────────────────────
void EditorApp::Init()
{
    GET_SINGLE(ResourceManager)->Init();

    _scene = std::make_shared<Scene>();
    _scene->SetName(L"Main Scene");

    _itemWindow = GET_SINGLE(WindowManager)->GetWindow<ItemWindow>(L"ItemWindow");
    _detailWindow = GET_SINGLE(WindowManager)->GetWindow<DetailWindow>(L"DetailWindow");

    // SceneSerializer에 Actor 팩토리 등록
    SceneSerializer::RegisterActor(L"SkySphereActor", [] { return std::make_shared<SkySphereActor>(); });
    SceneSerializer::RegisterActor(L"FloorActor", [] { return std::make_shared<FloorActor>(); });
    SceneSerializer::RegisterActor(L"CubeActor", [] { return std::make_shared<CubeActor>(); });
    SceneSerializer::RegisterActor(L"SphereActor", [] { return std::make_shared<SphereActor>(); });
    SceneSerializer::RegisterActor(L"CharacterActor", [] { return std::make_shared<CharacterActor>(); });
    SceneSerializer::RegisterActor(L"LightActor", [] { return std::make_shared<LightActor>(); });

    SpawnDefaultActors();
    CreateCamera();
    CreateHUD();

    GET_SINGLE(SceneManager)->ChangeScene(_scene);

    if (_itemWindow)   _itemWindow->SetScene(_scene);
    if (_detailWindow) _detailWindow->SetScene(_scene);

    RegisterActors();
}

void EditorApp::RegisterActors()
{
    if (!_itemWindow) return;

    _itemWindow->RegisterActor(L"SkySphere", [] { return std::make_shared<SkySphereActor>(); });
    _itemWindow->RegisterActor(L"Floor", [] { return std::make_shared<FloorActor>(); });
    _itemWindow->RegisterActor(L"Cube", [] { return std::make_shared<CubeActor>(); });
    _itemWindow->RegisterActor(L"Sphere", [] { return std::make_shared<SphereActor>(); });
    _itemWindow->RegisterActor(L"Character", [] { return std::make_shared<CharacterActor>(); });
}

void EditorApp::SpawnDefaultActors()
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

    auto lightActor = std::make_shared<LightActor>();
    lightActor->Spawn(_scene);
    _defaultActors.push_back(lightActor);
    if (auto lightComp = lightActor->GetEntity()->GetComponent<Light>())
        _scene->SetMainLight(lightComp);
}

void EditorApp::CreateHUD()
{
    auto hud = std::make_shared<Widget>(L"HUD");

    float cx = MAIN_WINDOW_WIDTH * 0.5f - 130.f;
    float cy = 12.f;
    hud->SetScreenPos(cx, cy);

    auto txt = std::make_shared<UIText>();
    txt->SetRect(0.f, 0.f, 260.f, 36.f);
    txt->SetFontSize(20);
    txt->SetTextGetter([]()
        {
            float t = GET_SINGLE(TimeManager)->GetTotalTime();
            wchar_t buf[64];
            swprintf_s(buf, L"Time  %02d:%02d", (int)(t / 60), (int)t % 60);
            return std::wstring(buf);
        });
    hud->AddUIComponent(txt);

    auto btn = std::make_shared<UIButton>();
    btn->SetRect(50.f, 44.f, 160.f, 28.f);
    btn->SetText(L"Reset Timer");
    btn->SetFontSize(15);
    btn->SetOnClick([]()
        {
            ::OutputDebugStringW(L"[UI] Reset Timer clicked!\n");
        });
    hud->AddUIComponent(btn);

    _scene->Add(hud);
}

void EditorApp::CreateCamera()
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

// ── Update ────────────────────────────────────────────────────────────────
void EditorApp::Update()
{
    CollisionManager::CheckCollision(_scene);
    UpdatePicking();

    // Entity 수 변화 감지 → 목록 Dirty
    if (_detailWindow && _scene)
    {
        static size_t prevEntityCount = 0;
        size_t curCount = _scene->GetEntities().size();
        if (curCount != prevEntityCount)
        {
            prevEntityCount = curCount;
            _detailWindow->MarkDirty();
            _detailWindow->RefreshEntityList();
        }
    }

    // Ctrl+S: 씬 저장
    if ((::GetKeyState(VK_CONTROL) & 0x8000) &&
        GET_SINGLE(InputManager)->GetButtonDown(KEY_TYPE::S))
    {
        SceneSerializer::Save(_scene, L"../Saved/scene.xml");
    }

    // Ctrl+L: 씬 로드
    if ((::GetKeyState(VK_CONTROL) & 0x8000) &&
        GET_SINGLE(InputManager)->GetButtonDown(KEY_TYPE::L))
    {
        SceneSerializer::Load(_scene, L"../Saved/scene.xml");
        if (_detailWindow) _detailWindow->MarkDirty();
    }

}

void EditorApp::Render() {}

// ── 피킹 ─────────────────────────────────────────────────────────────────
void EditorApp::UpdatePicking()
{
    if (GET_SINGLE(InputManager)->GetButtonDown(KEY_TYPE::DEL) ||
        GET_SINGLE(InputManager)->GetButtonDown(KEY_TYPE::P))
    {
        auto selected = _detailWindow ? _detailWindow->GetSelectedEntity() : nullptr;
        if (selected && _scene)
        {
            _scene->Remove(selected);
            _pickedEntity = nullptr;
            if (_detailWindow)
            {
                _detailWindow->ClearDetail();
                _detailWindow->MarkDirty();
                _detailWindow->RefreshEntityList();
            }
        }
    }

    // 메인 창이 활성 상태일 때만 피킹 처리
    if (!GET_SINGLE(InputManager)->IsMainWindowActive()) return;

    if (!GET_SINGLE(InputManager)->GetButtonDown(KEY_TYPE::LBUTTON)) return;

    POINT mp = GET_SINGLE(InputManager)->GetMousePos();

    auto picked = _scene->Pick((int32)mp.x, (int32)mp.y);
    _pickedEntity = picked;

    if (!picked)
    {
        if (_detailWindow) _detailWindow->ClearDetail();
        return;
    }

    if (!_detailWindow) return;
    DetailInfo info;
    FillDetailInfo(picked, info);
    _detailWindow->UpdateDetail(info);
    _detailWindow->SelectEntity(picked);
    if (!_detailWindow->IsVisible()) _detailWindow->Show();
}

void EditorApp::FillDetailInfo(std::shared_ptr<Entity> entity, DetailInfo& info)
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