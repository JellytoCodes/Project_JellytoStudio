#include "pch.h"
#include "BlockPlacer.h"

#include "Entity/Entity.h"
#include "Entity/Components/Transform.h"
#include "Entity/Components/MeshRenderer.h"
#include "Entity/Components/TileMap.h"
#include "Entity/Components/Collider/AABBCollider.h"
#include "Core/Managers/InputManager.h"
#include "Core/Managers/TimeManager.h"
#include "Scene/Scene.h"
#include "Scene/SceneManager.h"
#include "Scene/SceneSerializer.h"
#include "Resource/Managers/ResourceManager.h"
#include "Resource/Mesh.h"
#include "Resource/Material.h"
#include "Pipeline/Shader.h"

BlockPlacer::BlockPlacer()
    : MonoBehaviour()
{
}

void BlockPlacer::OnDestroy()
{
    HidePreview();
}

// ── 배치 모드 ─────────────────────────────────────────────────────────────

void BlockPlacer::SetPlacingMode(bool on)
{
    _placingMode = on;
    if (!on) HidePreview();
    ::OutputDebugStringW(on
        ? L"[BlockPlacer] 배치 모드 ON  (Tab: 끄기 | 좌클릭: 배치 | 우클릭: 제거 | Ctrl+S: 저장 | Ctrl+L: 로드)\n"
        : L"[BlockPlacer] 배치 모드 OFF\n");
}

// ── Update ────────────────────────────────────────────────────────────────

void BlockPlacer::Update()
{
    auto input = GET_SINGLE(InputManager);

    // Tab — 배치 모드 토글
    if (input->GetButtonDown(KEY_TYPE::TAB))
        SetPlacingMode(!_placingMode);

    // Ctrl+S — 씬 저장 (배치 모드 무관)
    if ((::GetKeyState(VK_CONTROL) & 0x8000) && input->GetButtonDown(KEY_TYPE::S))
    {
        auto scene = GET_SINGLE(SceneManager)->GetCurrentScene();
        SceneSerializer::Save(scene, _savePath, this);
    }

    // Ctrl+L — 씬 로드 (배치 모드 무관)
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

    if (input->GetButtonDown(KEY_TYPE::LBUTTON))
    {
        Vec3 groundPos;
        if (scene->PickGroundPoint((int32)mp.x, (int32)mp.y, groundPos, 0.f))
            TryPlace(groundPos);
        return;
    }

    if (input->GetButtonDown(KEY_TYPE::RBUTTON))
    {
        Vec3 groundPos;
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
    bool canPlace = tileMap->IsWalkable(col, row) && !IsCellOccupied(col, row);
    _previewValid = canPlace;
    snapped.y = 0.5f;

    // 프리뷰 Entity 최초 생성
    if (!_previewEntity)
    {
        _previewEntity = std::make_shared<Entity>(L"__BlockPreview__");
        _previewEntity->AddComponent(std::make_shared<Transform>());
        auto mr = std::make_shared<MeshRenderer>();
        mr->SetMesh(GET_SINGLE(ResourceManager)->Get<Mesh>(L"Cube"));
        mr->SetPass(0);
        _previewEntity->AddComponent(mr);
        scene->Add(_previewEntity);
    }

    _previewEntity->GetTransform()->SetLocalPosition(snapped);
    _previewEntity->GetTransform()->SetLocalScale(Vec3(1.f));

    if (auto mr = _previewEntity->GetComponent<MeshRenderer>())
    {
        if (canPlace)
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
    if (!tileMap)                           return false;
    if (!tileMap->IsValid(col, row))        return false;
    if (!tileMap->IsWalkable(col, row))     return false;
    if (IsCellOccupied(col, row))           return false;

    auto scene = GET_SINGLE(SceneManager)->GetCurrentScene();
    if (!scene) return false;

    Vec3 center = tileMap->GridToWorld(col, row);
    center.y = 0.5f;

    auto blockEntity = std::make_shared<Entity>(L"Block");
    blockEntity->AddComponent(std::make_shared<Transform>());
    blockEntity->GetTransform()->SetLocalPosition(center);
    blockEntity->GetTransform()->SetLocalScale(Vec3(1.f));

    auto mr = std::make_shared<MeshRenderer>();
    mr->SetMesh(GET_SINGLE(ResourceManager)->Get<Mesh>(L"Cube"));
    mr->SetPass(0);
    if (_blockMat)
        mr->SetMaterial(_blockMat);
    else if (auto mat = GET_SINGLE(ResourceManager)->Get<Material>(L"CubeMat"))
        mr->SetMaterial(mat);
    blockEntity->AddComponent(mr);

    auto col_ = std::make_shared<AABBCollider>();
    col_->SetBoxExtents(Vec3(0.5f));
    blockEntity->AddComponent(col_);

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

// ── ClearAllBlocks (IBlockPlacer) ─────────────────────────────────────────

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
    ::OutputDebugStringW(L"[BlockPlacer] 전체 블록 초기화\n");
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