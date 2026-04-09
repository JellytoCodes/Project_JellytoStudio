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

// ── Init ──────────────────────────────────────────────────────────────────
void EditorApp::Init()
{
	GET_SINGLE(ResourceManager)->Init();

	_scene = std::make_unique<Scene>();
	_scene->SetName(L"Main Scene");

	// WindowManager는 IWindow 소유 → GetWindow는 raw ptr 반환
	_itemWindow   = GET_SINGLE(WindowManager)->GetWindow<ItemWindow>(L"ItemWindow");
	_detailWindow = GET_SINGLE(WindowManager)->GetWindow<DetailWindow>(L"DetailWindow");

	SceneSerializer::RegisterActor(L"SkySphereActor", [] { return std::make_unique<SkySphereActor>(); });
	SceneSerializer::RegisterActor(L"FloorActor",     [] { return std::make_unique<FloorActor>(); });
	SceneSerializer::RegisterActor(L"CubeActor",      [] { return std::make_unique<CubeActor>(); });
	SceneSerializer::RegisterActor(L"SphereActor",    [] { return std::make_unique<SphereActor>(); });
	SceneSerializer::RegisterActor(L"CharacterActor", [] { return std::make_unique<CharacterActor>(); });
	SceneSerializer::RegisterActor(L"LightActor",     [] { return std::make_unique<LightActor>(); });

	SpawnDefaultActors();
	CreateCamera();
	CreateHUD();

	Scene* rawScene = _scene.get();
	GET_SINGLE(SceneManager)->ChangeScene(std::move(_scene));
	// _scene은 이제 nullptr — 이후 접근은 SceneManager 통해

	if (_itemWindow)   _itemWindow->SetScene(rawScene);
	if (_detailWindow) _detailWindow->SetScene(rawScene);

	RegisterActors();
}

void EditorApp::RegisterActors()
{
	if (!_itemWindow) return;

	_itemWindow->RegisterActor(L"SkySphere",  [] { return std::make_unique<SkySphereActor>(); });
	_itemWindow->RegisterActor(L"Floor",      [] { return std::make_unique<FloorActor>(); });
	_itemWindow->RegisterActor(L"Cube",       [] { return std::make_unique<CubeActor>(); });
	_itemWindow->RegisterActor(L"Sphere",     [] { return std::make_unique<SphereActor>(); });
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

	auto hud = std::make_unique<Widget>(L"HUD");
	float cx = MAIN_WINDOW_WIDTH * 0.5f - 130.f;
	hud->SetScreenPos(cx, 12.f);

	// 이전: make_shared<UIText>() → Widget이 shared_ptr 벡터로 보관
	// 변경: make_unique<UIText>() → Widget이 unique_ptr로 독점 소유
	auto txt = std::make_unique<UIText>();
	txt->SetRect(0.f, 0.f, 260.f, 36.f);
	txt->SetFontSize(20);
	txt->SetTextGetter([]()
		{
			float t = GET_SINGLE(TimeManager)->GetTotalTime();
			wchar_t buf[64];
			swprintf_s(buf, L"Time  %02d:%02d", (int)(t / 60), (int)t % 60);
			return std::wstring(buf);
		});
	hud->AddUIComponent(std::move(txt));

	auto btn = std::make_unique<UIButton>();
	btn->SetRect(50.f, 44.f, 160.f, 28.f);
	btn->SetText(L"Reset Timer");
	btn->SetFontSize(15);
	btn->SetOnClick([]() { ::OutputDebugStringW(L"[UI] Reset Timer clicked!\n"); });
	hud->AddUIComponent(std::move(btn));

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

// ── Update ────────────────────────────────────────────────────────────────
void EditorApp::Update()
{
	CollisionManager::CheckCollision();
	UpdatePicking();

	Scene* scene = GET_SINGLE(SceneManager)->GetCurrentScene();
	if (_detailWindow && scene)
	{
		static size_t prevEntityCount = 0;
		size_t curCount = scene->GetEntities().size();
		if (curCount != prevEntityCount)
		{
			prevEntityCount = curCount;
			_detailWindow->MarkDirty();
			_detailWindow->RefreshEntityList();
		}
	}

	if ((::GetKeyState(VK_CONTROL) & 0x8000) &&
		GET_SINGLE(InputManager)->GetButtonDown(KEY_TYPE::S))
	{
		SceneSerializer::Save(scene, L"../Saved/scene.xml");
	}

	if ((::GetKeyState(VK_CONTROL) & 0x8000) &&
		GET_SINGLE(InputManager)->GetButtonDown(KEY_TYPE::L))
	{
		SceneSerializer::Load(scene, L"../Saved/scene.xml");
		if (_detailWindow) _detailWindow->MarkDirty();
	}
}

void EditorApp::Render() {}

// ── 피킹 ─────────────────────────────────────────────────────────────────
void EditorApp::UpdatePicking()
{
	Scene* scene = GET_SINGLE(SceneManager)->GetCurrentScene();

	// Delete/P: 선택된 Entity 제거
	if (GET_SINGLE(InputManager)->GetButtonDown(KEY_TYPE::DEL) ||
		GET_SINGLE(InputManager)->GetButtonDown(KEY_TYPE::P))
	{
		// 이전: shared_ptr<Entity> selected = _detailWindow->GetSelectedEntity()
		// 변경: Entity* — 관찰자 포인터
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

	if (!GET_SINGLE(InputManager)->IsMainWindowActive()) return;
	if (!GET_SINGLE(InputManager)->GetButtonDown(KEY_TYPE::LBUTTON)) return;

	POINT mp = GET_SINGLE(InputManager)->GetMousePos();

	// 이전: shared_ptr<Entity> picked = _scene->Pick()
	// 변경: Entity* picked — 관찰자 포인터, refcount 0
	Entity* picked = scene ? scene->Pick((int32)mp.x, (int32)mp.y) : nullptr;
	_pickedEntity  = picked;

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

void EditorApp::FillDetailInfo(Entity* entity, DetailInfo& info)
{
	info.entityLabel = L"Entity";

	if (Transform* tf = entity->GetTransform())
	{
		Vec3 pos = tf->GetPosition(), rot = tf->GetRotation(), scl = tf->GetScale();
		info.tx = pos.x; info.ty = pos.y; info.tz = pos.z;
		info.rx = XMConvertToDegrees(rot.x);
		info.ry = XMConvertToDegrees(rot.y);
		info.rz = XMConvertToDegrees(rot.z);
		info.sx = scl.x; info.sy = scl.y; info.sz = scl.z;
	}

	if (ModelAnimator* animator = entity->GetComponent<ModelAnimator>())
	{
		if (Model* mdl = animator->GetModel())
		{
			info.entityLabel = info.modelName =
				(mdl->GetMeshCount() > 0) ? mdl->GetMeshByIndex(0)->name : L"Unknown";
			info.boneCount = (int)mdl->GetBoneCount();
			info.meshCount = (int)mdl->GetMeshCount();

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
}
