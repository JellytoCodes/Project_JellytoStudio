#include "pch.h"
#include "EditorApp.h"

#include "Actors.h"
#include "Resource/Managers/ResourceManager.h"
#include "Resource/Material.h"
#include "Resource/Mesh.h"
#include "Core/Managers/InputManager.h"
#include "Core/Managers/TimeManager.h"
#include "Scene/SceneManager.h"
#include "Scene/Scene.h"
#include "Entity/Entity.h"
#include "Entity/Actor.h"
#include "Entity/Components/Transform.h"
#include "Entity/Components/Camera.h"
#include "Entity/Components/MeshRenderer.h"
#include "Entity/Components/Collider/BaseCollider.h"
#include "Entity/Components/Collider/CollisionChannel.h"
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
#include "Core/DisplayContext.h"

EditorApp::EditorApp()  {}
EditorApp::~EditorApp() {}

void EditorApp::Init()
{
    GET_SINGLE(ResourceManager)->Init();

    _scene = std::make_unique<Scene>();
    _scene->SetName(L"Main Scene");

    _itemWindow       = GET_SINGLE(WindowManager)->GetWindow<ItemWindow>  (L"ItemWindow");
    _detailWindow     = GET_SINGLE(WindowManager)->GetWindow<DetailWindow>(L"DetailWindow");

    _chunkDebugWindow = GET_SINGLE(WindowManager)->RegisterWindow<ChunkDebugWindow>(L"ChunkDebugWindow");

    SceneSerializer::RegisterActor(L"SkySphereActor", [] { return std::make_unique<SkySphereActor>(); });
    SceneSerializer::RegisterActor(L"FloorActor",     [] { return std::make_unique<FloorActor>();     });
    SceneSerializer::RegisterActor(L"CubeActor",      [] { return std::make_unique<CubeActor>();      });
    SceneSerializer::RegisterActor(L"SphereActor",    [] { return std::make_unique<SphereActor>();    });
    SceneSerializer::RegisterActor(L"CharacterActor", [] { return std::make_unique<CharacterActor>(); });
    SceneSerializer::RegisterActor(L"LightActor",     [] { return std::make_unique<LightActor>();     });

    SpawnDefaultActors();
    CreateCamera();
    CreateHUD();

    Scene* rawScene = _scene.get();
    GET_SINGLE(SceneManager)->ChangeScene(std::move(_scene));

    if (_itemWindow)   _itemWindow  ->SetScene(rawScene);
    if (_detailWindow) _detailWindow->SetScene(rawScene);

    RegisterActors();
}

void EditorApp::RegisterActors()
{
    if (!_itemWindow) return;
    _itemWindow->RegisterActor(L"SkySphere",  [] { return std::make_unique<SkySphereActor>(); });
    _itemWindow->RegisterActor(L"Floor",      [] { return std::make_unique<FloorActor>();     });
    _itemWindow->RegisterActor(L"Cube",       [] { return std::make_unique<CubeActor>();      });
    _itemWindow->RegisterActor(L"Sphere",     [] { return std::make_unique<SphereActor>();    });
    _itemWindow->RegisterActor(L"Character",  [] { return std::make_unique<CharacterActor>(); });
}

void EditorApp::SpawnDefaultActors()
{
    Scene* scene = _scene.get();

    auto spawn = [&](std::unique_ptr<Actor> actor) -> Actor*
    {
        actor->Spawn(scene);
        Actor* raw = actor.get();
        _defaultActors.push_back(std::move(actor));
        return raw;
    };

    spawn(std::make_unique<SkySphereActor>());
    spawn(std::make_unique<FloorActor>());
    spawn(std::make_unique<CubeActor>());
    spawn(std::make_unique<SphereActor>());

    Actor* charActor = spawn(std::make_unique<CharacterActor>());
    _characterEntity = charActor->GetEntity();

    Actor* lightActor = spawn(std::make_unique<LightActor>());
    if (Light* lightComp = lightActor->GetEntity()->GetComponent<Light>())
        scene->SetMainLight(lightComp);
}

void EditorApp::CreateHUD()
{
    Scene* scene = _scene.get();
    float  cx    = GET_SINGLE(DisplayContext)->GetWidthF() * 0.5f - 130.f;

    auto hud = std::make_unique<Widget>(L"HUD");
    hud->SetScreenPos(cx, 12.f);

    auto timeText = std::make_unique<UIText>();
    timeText->SetRect(0.f, 0.f, 260.f, 36.f);
    timeText->SetFontSize(20);
    timeText->SetTextGetter([]()
    {
        const float t = GET_SINGLE(TimeManager)->GetTotalTime();
        wchar_t buf[64];
        swprintf_s(buf, L"Time  %02d:%02d", (int)(t / 60), (int)t % 60);
        return std::wstring(buf);
    });
    hud->AddUIComponent(std::move(timeText));

    auto saveText = std::make_unique<UIText>();
    saveText->SetRect(0.f, 44.f, 300.f, 22.f);
    saveText->SetFontSize(13);
    saveText->SetTextGetter([this]() { return _saveStatusMsg; });
    _saveStatusText = saveText.get();
    hud->AddUIComponent(std::move(saveText));

    auto hintText = std::make_unique<UIText>();
    hintText->SetRect(0.f, 70.f, 300.f, 18.f);
    hintText->SetFontSize(12);
    hintText->SetTextGetter([]() {
        return std::wstring(L"F2 : Chunk Debug  |  Ctrl+S : 저장  |  Ctrl+L : 로드  |  Del : 삭제");
    });
    hud->AddUIComponent(std::move(hintText));

    auto resetButton = std::make_unique<UIButton>();
    resetButton->SetRect(50.f, 94.f, 160.f, 28.f);
    resetButton->SetText(L"Reset Timer");
    resetButton->SetFontSize(15);
    resetButton->SetOnClick([]() {
        ::OutputDebugStringW(L"[UI] Reset Timer clicked!\n");
    });
    hud->AddUIComponent(std::move(resetButton));

    scene->Add(std::move(hud));
}

void EditorApp::CreateCamera()
{
    Scene* scene = _scene.get();

    auto cam = std::make_unique<Entity>(L"카메라");
    cam->AddComponent(std::make_unique<Transform>());
    cam->AddComponent(std::make_unique<Camera>());

    auto isoCtrl = std::make_unique<IsometricCameraController>();
    isoCtrl->SetDistance(20.f);
    isoCtrl->SetPanSpeed(10.f);
    isoCtrl->SetZoomSpeed(15.f);
    isoCtrl->SetMinDistance(5.f);
    isoCtrl->SetMaxDistance(60.f);

    _isoCamCtrl = isoCtrl.get();
    Camera* camComp = cam->GetComponent<Camera>();

    cam->AddComponent(std::move(isoCtrl));
    scene->SetMainCamera(camComp);
    scene->Add(std::move(cam));

    if (_characterEntity)
        _isoCamCtrl->SetTarget(_characterEntity);
}

void EditorApp::Update()
{
    CollisionManager::CheckCollision();
    UpdatePicking();

    const float dt    = GET_SINGLE(TimeManager)->GetDeltaTime();
    Scene*      scene = GET_SINGLE(SceneManager)->GetCurrentScene();

    if (_detailWindow && scene)
    {
        static size_t prevCount = 0;
        const size_t  curCount  = scene->GetEntities().size();
        if (curCount != prevCount)
        {
            prevCount = curCount;
            _detailWindow->MarkDirty();
            _detailWindow->RefreshEntityList();
        }
    }

    static bool prevF2 = false;
    const  bool curF2  = (::GetAsyncKeyState(VK_F2) & 0x8000) != 0;
    if (curF2 && !prevF2 && _chunkDebugWindow)
        _chunkDebugWindow->Toggle();
    prevF2 = curF2;

    if (_chunkDebugWindow && _chunkDebugWindow->IsVisible())
    {
        _chunkRefreshTimer += dt;
        if (_chunkRefreshTimer >= kChunkRefreshInterval)
        {
            _chunkRefreshTimer = 0.f;

            Entity* selected = _detailWindow
                ? _detailWindow->GetSelectedEntity()
                : nullptr;

            _chunkDebugWindow->Refresh(selected);
        }
    }

    if ((::GetKeyState(VK_CONTROL) & 0x8000) &&
        GET_SINGLE(InputManager)->GetButtonDown(KEY_TYPE::S))
    {
        const bool ok = SceneSerializer::Save(scene, L"../Saved/scene.json");
        SetSaveStatus(ok ? L"✔ 저장 완료 — scene.json" : L"✘ 저장 실패");
    }

    if ((::GetKeyState(VK_CONTROL) & 0x8000) &&
        GET_SINGLE(InputManager)->GetButtonDown(KEY_TYPE::L))
    {
        const bool ok = SceneSerializer::Load(scene, L"../Saved/scene.json");
        SetSaveStatus(ok ? L"✔ 로드 완료 — scene.json" : L"✘ 로드 실패 — 파일 없음");
        if (_detailWindow) _detailWindow->MarkDirty();
    }

    if (!_saveStatusMsg.empty())
    {
        _saveStatusTimer += dt;
        if (_saveStatusTimer >= kSaveStatusDuration)
        {
            _saveStatusMsg.clear();
            _saveStatusTimer = 0.f;
        }
    }
}

void EditorApp::Render() {}

void EditorApp::UpdatePicking()
{
    Scene* scene = GET_SINGLE(SceneManager)->GetCurrentScene();

    if (GET_SINGLE(InputManager)->GetButtonDown(KEY_TYPE::DEL))
    {
        Entity* selected = _detailWindow ? _detailWindow->GetSelectedEntity() : nullptr;
        if (selected && scene)
        {
            scene->Remove(selected);
            _pickedEntity = nullptr;
            if (_detailWindow)
            {
                _detailWindow->ClearDetail();
                _detailWindow->MarkDirty();
                _detailWindow->RefreshEntityList();
            }
        }
    }

    if (!GET_SINGLE(InputManager)->IsMainWindowActive())           return;
    if (!GET_SINGLE(InputManager)->GetButtonDown(KEY_TYPE::LBUTTON)) return;

    const POINT mp     = GET_SINGLE(InputManager)->GetMousePos();
    Entity*     picked = scene ? scene->Pick((int32)mp.x, (int32)mp.y) : nullptr;
    _pickedEntity      = picked;

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

    if (_chunkDebugWindow && _chunkDebugWindow->IsVisible())
    {
        _chunkRefreshTimer = kChunkRefreshInterval;
    }
}

void EditorApp::FillDetailInfo(Entity* entity, DetailInfo& info)
{
    info.entityLabel = entity->GetEntityName();

    if (ModelAnimator* animator = entity->GetComponent<ModelAnimator>())
    {
        info.rendererType = L"ModelAnimator";
        if (auto mdl = animator->GetModel())
        {
            info.modelName = (mdl->GetMeshCount() > 0)
                ? mdl->GetMeshByIndex(0)->name : L"Unknown";
            info.entityLabel = info.modelName;
            info.meshName    = info.modelName;
            info.boneCount   = (int)mdl->GetBoneCount();
            info.meshCount   = (int)mdl->GetMeshCount();

            if (mdl->GetAnimationCount() > 0)
            {
                ModelAnimation* anim = mdl->GetAnimationByIndex(0);
                info.animName   = anim->name;
                info.frameCount = (int)anim->frameCount;
                info.frameRate  = anim->frameRate;
                info.duration   = anim->duration;
            }
        }
    }
    else if (MeshRenderer* renderer = entity->GetComponent<MeshRenderer>())
    {
        info.rendererType = L"MeshRenderer";
        if (auto mesh = renderer->GetMesh())     info.meshName     = mesh->GetName();
        if (auto mat  = renderer->GetMaterial()) info.materialName = mat->GetName();
    }
    else
    {
        info.rendererType = L"없음";
    }

    if (BaseCollider* col = entity->GetComponent<BaseCollider>())
    {
        auto ShapeStr = [](ColliderType t) -> std::wstring
        {
            switch (t)
            {
            case ColliderType::AABB:    return L"AABB (Box)";
            case ColliderType::OBB:     return L"OBB (Box·회전)";
            case ColliderType::Sphere:  return L"Sphere";
            case ColliderType::Frustum: return L"Frustum";
            default:                    return L"알 수 없음";
            }
        };

        auto MaskStr = [](uint8 mask) -> std::wstring
        {
            if (mask == 0)    return L"None";
            if (mask == 0xFF) return L"All";
            static const std::pair<CollisionChannel, const wchar_t*> kMap[] = {
                { CollisionChannel::Default,   L"Default"   },
                { CollisionChannel::Character, L"Character" },
                { CollisionChannel::Priming,   L"Priming"   },
                { CollisionChannel::Mushroom,  L"Mushroom"  },
                { CollisionChannel::Floor,     L"Floor"     },
            };
            std::wstring r;
            for (auto& [ch, name] : kMap)
                if (ChannelInMask(ch, mask)) { if (!r.empty()) r += L" | "; r += name; }
            return r.empty() ? L"None" : r;
        };

        info.hasCollider      = true;
        info.colliderShape    = ShapeStr(col->GetColliderType());
        info.ownChannel       = [&]() -> std::wstring {
            switch (col->GetOwnChannel()) {
            case CollisionChannel::Default:   return L"Default";
            case CollisionChannel::Character: return L"Character";
            case CollisionChannel::Priming:   return L"Priming";
            case CollisionChannel::Mushroom:  return L"Mushroom";
            case CollisionChannel::Floor:     return L"Floor";
            default:                          return L"None";
            }
        }();
        info.pickableChannels = MaskStr(col->GetPickableMask());
        info.isStatic         = col->IsStatic();

        const Vec3 ext = col->GetOffsetScale();
        info.extX = ext.x; info.extY = ext.y; info.extZ = ext.z;
    }

    if (Transform* tf = entity->GetComponent<Transform>())
    {
        const Vec3 pos = tf->GetPosition();
        const Vec3 rot = tf->GetRotation();
        const Vec3 scl = tf->GetScale();

        info.tx = pos.x; info.ty = pos.y; info.tz = pos.z;
        info.rx = XMConvertToDegrees(rot.x);
        info.ry = XMConvertToDegrees(rot.y);
        info.rz = XMConvertToDegrees(rot.z);
        info.sx = scl.x; info.sy = scl.y; info.sz = scl.z;
    }
}

void EditorApp::SetSaveStatus(const std::wstring& msg)
{
    _saveStatusMsg   = msg;
    _saveStatusTimer = 0.f;
    ::OutputDebugStringW((msg + L"\n").c_str());
}