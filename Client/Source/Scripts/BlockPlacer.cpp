#include "pch.h"
#include "BlockPlacer.h"

#include "Entity/Entity.h"
#include "Entity/Components/Transform.h"
#include "Entity/Components/MeshRenderer.h"
#include "Entity/Components/TileMap.h"
#include "Entity/Components/Collider/AABBCollider.h"
#include "Entity/Components/Collider/SphereCollider.h"
#include "Core/Managers/InputManager.h"
#include "Scene/Scene.h"
#include "Scene/SceneManager.h"
#include "Scene/SceneSerializer.h"
#include "Resource/Managers/ResourceManager.h"
#include "Resource/Mesh.h"
#include "Resource/Material.h"
#include "Pipeline/Shader.h"

using SlotType = PaletteWidget::SlotType;

BlockPlacer::BlockPlacer() : MonoBehaviour() {}

void BlockPlacer::OnDestroy() { HidePreview(); }

// ── 슬롯별 배치 파라미터 ──────────────────────────────────────────────────

BlockPlacer::PlaceParams BlockPlacer::GetPlaceParams(SlotType type) const
{
    switch (type)
    {
    case SlotType::BlockNormal: return { L"Cube",   Vec3(1.f, 1.f, 1.f),   0.5f, Vec3(0.5f, 0.5f, 0.5f) };
    case SlotType::BlockFlat:   return { L"Cube",   Vec3(1.f, 0.5f, 1.f),  0.25f,Vec3(0.5f, 0.25f, 0.5f) };
    case SlotType::BlockLarge:  return { L"Cube",   Vec3(2.f, 1.f, 2.f),   0.5f, Vec3(1.f, 0.5f, 1.f) };
    case SlotType::Sphere:      return { L"Sphere", Vec3(0.9f, 0.9f, 0.9f),0.45f,Vec3(0.45f) };
    default:                    return { L"Cube",   Vec3(1.f),              0.5f, Vec3(0.5f) };
    }
}

std::shared_ptr<Material> BlockPlacer::GetSlotMaterial(SlotType type)
{
    int idx = static_cast<int>(type);
    if (_slotMats[idx]) return _slotMats[idx];

    // 슬롯별 색상 정의 (ambient/diffuse)
    static const Vec4 colors[] = {
        Vec4(0.80f, 0.75f, 0.65f, 1.f),  // BlockNormal: 베이지
        Vec4(0.55f, 0.70f, 0.85f, 1.f),  // BlockFlat:   하늘색
        Vec4(0.50f, 0.75f, 0.55f, 1.f),  // BlockLarge:  초록
        Vec4(0.80f, 0.55f, 0.75f, 1.f),  // Sphere:      보라
        Vec4(1.f,   0.3f,  0.3f,  1.f),  // Eraser:      빨강 (미사용)
    };

    auto shader = std::make_shared<Shader>(L"../Engine/Shaders/Terrain.hlsl");
    auto mat = std::make_shared<Material>();
    mat->SetShader(shader);
    if (idx < 5)
    {
        auto& d = mat->GetMaterialDesc();
        d.ambient = d.diffuse = colors[idx];
        d.specular = Vec4(0.3f, 0.3f, 0.3f, 1.f);
    }
    _slotMats[idx] = mat;
    return mat;
}

// ── 모드 전환 ─────────────────────────────────────────────────────────────

void BlockPlacer::SetPlacingMode(bool on)
{
    _placingMode = on;
    if (!on) HidePreview();

    // 팔레트 동기화
    if (auto palette = _palette.lock())
        palette->SetPlacingMode(on);

    ::OutputDebugStringW(on
        ? L"[BlockPlacer] ON  (좌클릭:배치 | 우클릭:제거 | Ctrl+S:저장 | Ctrl+L:로드)\n"
        : L"[BlockPlacer] OFF\n");
}

// ── Update ────────────────────────────────────────────────────────────────

void BlockPlacer::Update()
{
    auto input = GET_SINGLE(InputManager);

    // Tab — 배치 모드 토글
    if (input->GetButtonDown(KEY_TYPE::TAB))
        SetPlacingMode(!_placingMode);

    // Ctrl+S — 저장
    if ((::GetKeyState(VK_CONTROL) & 0x8000) && input->GetButtonDown(KEY_TYPE::S))
    {
        auto scene = GET_SINGLE(SceneManager)->GetCurrentScene();
        SceneSerializer::Save(scene, _savePath, this);
    }

    // Ctrl+L — 로드
    if ((::GetKeyState(VK_CONTROL) & 0x8000) && input->GetButtonDown(KEY_TYPE::L))
    {
        auto scene = GET_SINGLE(SceneManager)->GetCurrentScene();
        SceneSerializer::Load(scene, _savePath, this);
    }

    if (!_placingMode) { HidePreview(); return; }

    HandleInput();
    UpdatePreview();
}

// ── 입력 처리 ─────────────────────────────────────────────────────────────

void BlockPlacer::HandleInput()
{
    auto input = GET_SINGLE(InputManager);
    if (!input->IsMainWindowActive()) return;

    auto scene = GET_SINGLE(SceneManager)->GetCurrentScene();
    if (!scene) return;

    POINT mp = input->GetMousePos();
    Vec3 groundPos;

    // 좌클릭 — 팔레트 슬롯에 따라 배치 or 제거
    if (input->GetButtonDown(KEY_TYPE::LBUTTON))
    {
        if (!scene->PickGroundPoint((int32)mp.x, (int32)mp.y, groundPos, 0.f)) return;

        // Eraser 슬롯이면 제거
        auto palette = _palette.lock();
        if (palette && palette->GetSelectedSlotType() == SlotType::Eraser)
            TryRemove(groundPos);
        else
            TryPlace(groundPos);
        return;
    }

    // 우클릭 — 항상 제거
    if (input->GetButtonDown(KEY_TYPE::RBUTTON))
    {
        if (scene->PickGroundPoint((int32)mp.x, (int32)mp.y, groundPos, 0.f))
            TryRemove(groundPos);
    }
}

// ── 프리뷰 ────────────────────────────────────────────────────────────────

void BlockPlacer::UpdatePreview()
{
    auto scene = GET_SINGLE(SceneManager)->GetCurrentScene();
    auto tileMap = FindTileMap();
    if (!scene || !tileMap) { HidePreview(); return; }

    POINT mp = GET_SINGLE(InputManager)->GetMousePos();
    Vec3 groundPos;
    if (!scene->PickGroundPoint((int32)mp.x, (int32)mp.y, groundPos, 0.f))
    {
        HidePreview(); return;
    }

    Vec3 snapped;
    if (!tileMap->SnapToGrid(groundPos, snapped))
    {
        HidePreview(); return;
    }

    int32 col, row;
    tileMap->WorldToGrid(snapped, col, row);

    // Eraser 모드면 배치 가능 조건 반전
    auto palette = _palette.lock();
    bool isEraser = palette && palette->GetSelectedSlotType() == SlotType::Eraser;
    bool canAct = isEraser ? IsCellOccupied(col, row)
        : (tileMap->IsWalkable(col, row) && !IsCellOccupied(col, row));

    _previewValid = canAct;

    // 현재 슬롯의 배치 파라미터 가져오기
    SlotType slotType = palette ? palette->GetSelectedSlotType() : SlotType::BlockNormal;
    auto params = GetPlaceParams(slotType);
    snapped.y = params.yOffset;

    // 프리뷰 Entity 생성
    if (!_previewEntity)
    {
        _previewEntity = std::make_shared<Entity>(L"__Preview__");
        _previewEntity->AddComponent(std::make_shared<Transform>());
        auto mr = std::make_shared<MeshRenderer>();
        mr->SetPass(0);
        _previewEntity->AddComponent(mr);
        scene->Add(_previewEntity);
    }

    _previewEntity->GetTransform()->SetLocalPosition(snapped);
    _previewEntity->GetTransform()->SetLocalScale(params.scale);

    if (auto mr = _previewEntity->GetComponent<MeshRenderer>())
    {
        mr->SetMesh(GET_SINGLE(ResourceManager)->Get<Mesh>(params.meshKey));

        // 배치 가능: 초록 / 불가: 빨강
        if (canAct)
        {
            if (!_previewMatOk)
            {
                _previewMatOk = std::make_shared<Material>();
                _previewMatOk->SetShader(std::make_shared<Shader>(L"../Engine/Shaders/Terrain.hlsl"));
                auto& d = _previewMatOk->GetMaterialDesc();
                d.ambient = d.diffuse = Vec4(0.2f, 0.9f, 0.2f, 0.5f);
                d.specular = Vec4(0, 0, 0, 0);
            }
            mr->SetMaterial(_previewMatOk);
        }
        else
        {
            if (!_previewMatBad)
            {
                _previewMatBad = std::make_shared<Material>();
                _previewMatBad->SetShader(std::make_shared<Shader>(L"../Engine/Shaders/Terrain.hlsl"));
                auto& d = _previewMatBad->GetMaterialDesc();
                d.ambient = d.diffuse = Vec4(0.9f, 0.2f, 0.2f, 0.5f);
                d.specular = Vec4(0, 0, 0, 0);
            }
            mr->SetMaterial(_previewMatBad);
        }
    }
}

void BlockPlacer::HidePreview()
{
    if (!_previewEntity) return;
    if (auto scene = GET_SINGLE(SceneManager)->GetCurrentScene())
        scene->Remove(_previewEntity);
    _previewEntity.reset();
    _previewValid = false;
}

// ── 배치 ─────────────────────────────────────────────────────────────────

bool BlockPlacer::TryPlace(const Vec3& worldPos)
{
    auto tileMap = FindTileMap();
    if (!tileMap) return false;
    int32 col, row;
    if (!tileMap->WorldToGrid(worldPos, col, row)) return false;
    return PlaceBlock(col, row);
}

bool BlockPlacer::PlaceBlock(int32 col, int32 row)
{
    auto tileMap = FindTileMap();
    if (!tileMap)                       return false;
    if (!tileMap->IsValid(col, row))    return false;
    if (!tileMap->IsWalkable(col, row)) return false;
    if (IsCellOccupied(col, row))       return false;

    auto scene = GET_SINGLE(SceneManager)->GetCurrentScene();
    if (!scene) return false;

    // 현재 팔레트 슬롯 타입 확인
    auto palette = _palette.lock();
    SlotType type = palette ? palette->GetSelectedSlotType() : SlotType::BlockNormal;
    if (type == SlotType::Eraser) return false; // 지우개 슬롯은 배치 불가

    auto params = GetPlaceParams(type);
    Vec3 center = tileMap->GridToWorld(col, row);
    center.y = params.yOffset;

    // Entity 생성
    auto blockEntity = std::make_shared<Entity>(L"Block");
    blockEntity->AddComponent(std::make_shared<Transform>());
    blockEntity->GetTransform()->SetLocalPosition(center);
    blockEntity->GetTransform()->SetLocalScale(params.scale);

    // MeshRenderer
    auto mr = std::make_shared<MeshRenderer>();
    mr->SetMesh(GET_SINGLE(ResourceManager)->Get<Mesh>(params.meshKey));
    mr->SetPass(0);
    mr->SetMaterial(GetSlotMaterial(type));
    blockEntity->AddComponent(mr);

    // Collider
    if (params.meshKey == L"Sphere")
    {
        auto col_ = std::make_shared<SphereCollider>();
        col_->SetRadius(params.extents.x);
        blockEntity->AddComponent(col_);
    }
    else
    {
        auto col_ = std::make_shared<AABBCollider>();
        col_->SetBoxExtents(params.extents);
        blockEntity->AddComponent(col_);
    }

    scene->Add(blockEntity);
    tileMap->SetWalkable(col, row, false);

    uint64 key = (uint64)col * 10000 + (uint64)row;
    _blockEntities[key] = blockEntity;
    _placedCells.emplace_back(col, row);
    return true;
}

// ── 제거 ─────────────────────────────────────────────────────────────────

bool BlockPlacer::TryRemove(const Vec3& worldPos)
{
    auto tileMap = FindTileMap();
    if (!tileMap) return false;
    int32 col, row;
    if (!tileMap->WorldToGrid(worldPos, col, row)) return false;
    return RemoveBlock(col, row);
}

bool BlockPlacer::RemoveBlock(int32 col, int32 row)
{
    uint64 key = (uint64)col * 10000 + (uint64)row;
    auto it = _blockEntities.find(key);
    if (it == _blockEntities.end()) return false;

    if (auto scene = GET_SINGLE(SceneManager)->GetCurrentScene())
        scene->Remove(it->second);
    if (auto tileMap = FindTileMap())
        tileMap->SetWalkable(col, row, true);

    _blockEntities.erase(it);
    _placedCells.erase(
        std::remove_if(_placedCells.begin(), _placedCells.end(),
            [col, row](const auto& p) { return p.first == col && p.second == row; }),
        _placedCells.end());
    return true;
}

void BlockPlacer::ClearAllBlocks()
{
    auto scene = GET_SINGLE(SceneManager)->GetCurrentScene();
    auto tileMap = FindTileMap();
    for (auto& [key, entity] : _blockEntities)
    {
        if (scene) scene->Remove(entity);
        if (tileMap)
        {
            int32 col = (int32)(key / 10000);
            int32 row = (int32)(key % 10000);
            tileMap->SetWalkable(col, row, true);
        }
    }
    _blockEntities.clear();
    _placedCells.clear();
}

// ── 유틸 ─────────────────────────────────────────────────────────────────

bool BlockPlacer::IsCellOccupied(int32 col, int32 row) const
{
    return _blockEntities.count((uint64)col * 10000 + (uint64)row) > 0;
}

std::shared_ptr<TileMap> BlockPlacer::FindTileMap() const
{
    auto scene = GET_SINGLE(SceneManager)->GetCurrentScene();
    if (!scene) return nullptr;
    for (auto& entity : scene->GetEntities())
        if (auto tm = entity->GetComponent<TileMap>()) return tm;
    return nullptr;
}