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

// ── 슬롯 파라미터 ─────────────────────────────────────────────────────────

BlockPlacer::PlaceParams BlockPlacer::GetPlaceParams(SlotType type) const
{
    switch (type)
    {
    case SlotType::BlockNormal: return { L"Cube",   Vec3(1.f,  1.f,  1.f),  1.0f, Vec3(0.5f,  0.5f,  0.5f)  };
    case SlotType::BlockFlat:   return { L"Cube",   Vec3(1.f,  0.5f, 1.f),  0.5f, Vec3(0.5f,  0.25f, 0.5f)  };
    case SlotType::BlockLarge:  return { L"Cube",   Vec3(2.f,  1.f,  2.f),  1.0f, Vec3(1.f,   0.5f,  1.f)   };
    case SlotType::Sphere:      return { L"Sphere", Vec3(0.9f, 0.9f, 0.9f), 0.9f, Vec3(0.45f, 0.45f, 0.45f) };
    default:                    return { L"Cube",   Vec3(1.f,  1.f,  1.f),  1.0f, Vec3(0.5f,  0.5f,  0.5f)  };
    }
}

std::shared_ptr<Material> BlockPlacer::GetSlotMaterial(SlotType type)
{
    int idx = static_cast<int>(type);
    if (_slotMats[idx]) return _slotMats[idx];

    static const Vec4 colors[] = {
        Vec4(0.80f, 0.75f, 0.65f, 1.f),
        Vec4(0.55f, 0.70f, 0.85f, 1.f),
        Vec4(0.50f, 0.75f, 0.55f, 1.f),
        Vec4(0.80f, 0.55f, 0.75f, 1.f),
        Vec4(1.f,   0.3f,  0.3f,  1.f),
    };

    auto mat = std::make_shared<Material>();
    mat->SetShader(std::make_shared<Shader>(L"../Engine/Shaders/Terrain.hlsl"));
    auto& d = mat->GetMaterialDesc();
    d.ambient  = colors[idx < 5 ? idx : 0];
    d.diffuse  = d.ambient;
    d.specular = Vec4(0.3f, 0.3f, 0.3f, 1.f);
    d.emissive = Vec4(0.f);
    _slotMats[idx] = mat;
    return mat;
}

// ── 레이어 탐색 (쌓기) ────────────────────────────────────────────────────

bool BlockPlacer::FindNextLayer(int32 col, int32 row, const PlaceParams& params,
                                int32& outLayer, float& outY) const
{
    // layer 0부터 LAYER_MAX-1까지 빈 레이어 탐색
    for (int32 layer = 0; layer < LAYER_MAX; layer++)
    {
        if (!IsCellLayerOccupied(col, row, layer))
        {
            outLayer = layer;
            // Y 위치: 각 레이어는 해당 높이만큼 쌓임
            // layer 0: y = blockHeight/2 (바닥 위 중심)
            // layer 1: y = blockHeight * 1 + blockHeight/2, ...
            outY = params.blockHeight * layer + params.blockHeight * 0.5f;
            return true;
        }
    }
    return false; // 꽉 참
}

// ── 캐릭터 겹침 검사 ──────────────────────────────────────────────────────

bool BlockPlacer::IsOverlappingCharacter(const Vec3& center, const Vec3& halfExtents) const
{
    auto charEntity = _character.lock();
    if (!charEntity) return false;

    auto charCollider = charEntity->GetComponent<AABBCollider>();
    if (!charCollider) return false;

    const BoundingBox& charBox = charCollider->GetBoundingBox();

    // 블록의 BoundingBox와 캐릭터 BoundingBox AABB 교차 검사
    BoundingBox blockBox;
    blockBox.Center  = center;
    blockBox.Extents = halfExtents;

    return blockBox.Intersects(charBox);
}

// ── 모드 전환 ─────────────────────────────────────────────────────────────

void BlockPlacer::SetPlacingMode(bool on)
{
    _placingMode = on;
    if (!on) HidePreview();
    if (auto p = _palette.lock()) p->SetPlacingMode(on);
    ::OutputDebugStringW(on ? L"[BlockPlacer] ON\n" : L"[BlockPlacer] OFF\n");
}

// ── Update ────────────────────────────────────────────────────────────────

void BlockPlacer::Update()
{
    auto input = GET_SINGLE(InputManager);

    if (input->GetButtonDown(KEY_TYPE::TAB))
        SetPlacingMode(!_placingMode);

    if ((::GetKeyState(VK_CONTROL) & 0x8000) && input->GetButtonDown(KEY_TYPE::S))
        SceneSerializer::Save(GET_SINGLE(SceneManager)->GetCurrentScene(), _savePath, this);

    if ((::GetKeyState(VK_CONTROL) & 0x8000) && input->GetButtonDown(KEY_TYPE::L))
        SceneSerializer::Load(GET_SINGLE(SceneManager)->GetCurrentScene(), _savePath, this);

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

    if (input->GetButtonDown(KEY_TYPE::LBUTTON))
    {
        if (!scene->PickGroundPoint((int32)mp.x, (int32)mp.y, groundPos, 0.f)) return;
        auto palette = _palette.lock();
        if (palette && palette->GetSelectedSlotType() == SlotType::Eraser)
            TryRemove(groundPos);
        else
            TryPlace(groundPos);
        return;
    }

    if (input->GetButtonDown(KEY_TYPE::RBUTTON))
    {
        if (scene->PickGroundPoint((int32)mp.x, (int32)mp.y, groundPos, 0.f))
            TryRemove(groundPos);
    }
}

// ── 프리뷰 ────────────────────────────────────────────────────────────────

void BlockPlacer::UpdatePreview()
{
    auto scene   = GET_SINGLE(SceneManager)->GetCurrentScene();
    auto tileMap = FindTileMap();
    if (!scene || !tileMap) { HidePreview(); return; }

    POINT mp = GET_SINGLE(InputManager)->GetMousePos();
    Vec3 groundPos;
    if (!scene->PickGroundPoint((int32)mp.x, (int32)mp.y, groundPos, 0.f))
    { HidePreview(); return; }

    Vec3 snapped;
    if (!tileMap->SnapToGrid(groundPos, snapped))
    { HidePreview(); return; }

    int32 col, row;
    tileMap->WorldToGrid(snapped, col, row);

    auto palette  = _palette.lock();
    SlotType slotType = palette ? palette->GetSelectedSlotType() : SlotType::BlockNormal;
    bool isEraser = (slotType == SlotType::Eraser);

    bool canAct = false;
    Vec3 previewPos = snapped;
    auto params = GetPlaceParams(slotType);

    if (isEraser)
    {
        canAct = IsCellOccupied(col, row);
        previewPos.y = 0.5f;
    }
    else
    {
        int32 layer; float yPos;
        if (tileMap->IsValid(col, row) && FindNextLayer(col, row, params, layer, yPos))
        {
            previewPos.y = yPos;
            Vec3 halfExtents = params.extents;
            // 캐릭터 겹침 체크
            bool charOverlap = IsOverlappingCharacter(previewPos, halfExtents);
            canAct = !charOverlap;
        }
    }

    _previewValid = canAct;

    // 프리뷰 Entity 생성/갱신
    if (!_previewEntity)
    {
        _previewEntity = std::make_shared<Entity>(L"__Preview__");
        _previewEntity->AddComponent(std::make_shared<Transform>());
        auto mr = std::make_shared<MeshRenderer>();
        mr->SetPass(0);
        _previewEntity->AddComponent(mr);
        scene->Add(_previewEntity);
    }

    _previewEntity->GetTransform()->SetLocalPosition(previewPos);
    _previewEntity->GetTransform()->SetLocalScale(params.scale);

    if (auto mr = _previewEntity->GetComponent<MeshRenderer>())
    {
        mr->SetMesh(GET_SINGLE(ResourceManager)->Get<Mesh>(params.meshKey));

        if (canAct)
        {
            if (!_previewMatOk)
            {
                _previewMatOk = std::make_shared<Material>();
                _previewMatOk->SetShader(std::make_shared<Shader>(L"../Engine/Shaders/Terrain.hlsl"));
                auto& d = _previewMatOk->GetMaterialDesc();
                d.ambient = d.diffuse = Vec4(0.2f, 0.9f, 0.2f, 0.5f);
                d.specular = Vec4(0,0,0,0); d.emissive = Vec4(0,0,0,0);
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
                d.specular = Vec4(0,0,0,0); d.emissive = Vec4(0,0,0,0);
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
    if (!tileMap || !tileMap->IsValid(col, row)) return false;

    auto scene = GET_SINGLE(SceneManager)->GetCurrentScene();
    if (!scene) return false;

    auto palette  = _palette.lock();
    SlotType type = palette ? palette->GetSelectedSlotType() : SlotType::BlockNormal;
    if (type == SlotType::Eraser) return false;

    auto params = GetPlaceParams(type);

    // 다음 빈 레이어 탐색 (쌓기 지원)
    int32 layer; float yPos;
    if (!FindNextLayer(col, row, params, layer, yPos)) return false;

    Vec3 center = tileMap->GridToWorld(col, row);
    center.y    = yPos;

    // 캐릭터 겹침 체크
    if (IsOverlappingCharacter(center, params.extents))
    {
        ::OutputDebugStringW(L"[BlockPlacer] 캐릭터 겹침 — 배치 불가\n");
        return false;
    }

    // Entity 생성
    auto blockEntity = std::make_shared<Entity>(L"Block");
    blockEntity->AddComponent(std::make_shared<Transform>());
    blockEntity->GetTransform()->SetLocalPosition(center);
    blockEntity->GetTransform()->SetLocalScale(params.scale);

    auto mr = std::make_shared<MeshRenderer>();
    mr->SetMesh(GET_SINGLE(ResourceManager)->Get<Mesh>(params.meshKey));
    mr->SetPass(0);
    mr->SetMaterial(GetSlotMaterial(type));
    blockEntity->AddComponent(mr);

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

    // layer 0에만 walkable=false (캐릭터 이동 차단)
    if (layer == 0)
        tileMap->SetWalkable(col, row, false);

    uint64 key = MakeKey(col, row, layer);
    _blockEntities[key] = blockEntity;
    _placedCells.emplace_back(col, row);

    wchar_t dbg[128];
    swprintf_s(dbg, L"[BlockPlacer] 배치 (%d,%d) layer=%d y=%.2f\n", col, row, layer, yPos);
    ::OutputDebugStringW(dbg);
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
    // 가장 높은 레이어부터 제거 (LIFO)
    for (int32 layer = LAYER_MAX - 1; layer >= 0; layer--)
    {
        uint64 key = MakeKey(col, row, layer);
        auto it = _blockEntities.find(key);
        if (it == _blockEntities.end()) continue;

        if (auto scene = GET_SINGLE(SceneManager)->GetCurrentScene())
            scene->Remove(it->second);

        // layer 0을 제거하면 walkable 복원
        if (layer == 0)
        {
            if (auto tileMap = FindTileMap())
                tileMap->SetWalkable(col, row, true);
        }

        _blockEntities.erase(it);
        _placedCells.erase(
            std::remove_if(_placedCells.begin(), _placedCells.end(),
                [col, row](const auto& p){ return p.first == col && p.second == row; }),
            _placedCells.end());

        wchar_t dbg[64];
        swprintf_s(dbg, L"[BlockPlacer] 제거 (%d,%d) layer=%d\n", col, row, layer);
        ::OutputDebugStringW(dbg);
        return true;
    }
    return false;
}

void BlockPlacer::ClearAllBlocks()
{
    auto scene   = GET_SINGLE(SceneManager)->GetCurrentScene();
    auto tileMap = FindTileMap();
    for (auto& [key, entity] : _blockEntities)
    {
        if (scene) scene->Remove(entity);
        if (tileMap)
        {
            int32 col   = (int32)(key / 1000000);
            int32 row   = (int32)((key % 1000000) / 100);
            int32 layer = (int32)(key % 100);
            if (layer == 0) tileMap->SetWalkable(col, row, true);
        }
    }
    _blockEntities.clear();
    _placedCells.clear();
    ::OutputDebugStringW(L"[BlockPlacer] 전체 블록 초기화\n");
}

// ── 유틸 ─────────────────────────────────────────────────────────────────

bool BlockPlacer::IsCellLayerOccupied(int32 col, int32 row, int32 layer) const
{
    return _blockEntities.count(MakeKey(col, row, layer)) > 0;
}

bool BlockPlacer::IsCellOccupied(int32 col, int32 row) const
{
    // layer 0이 있으면 점유된 것으로 간주
    return IsCellLayerOccupied(col, row, 0);
}

std::shared_ptr<TileMap> BlockPlacer::FindTileMap() const
{
    auto scene = GET_SINGLE(SceneManager)->GetCurrentScene();
    if (!scene) return nullptr;
    for (auto& entity : scene->GetEntities())
        if (auto tm = entity->GetComponent<TileMap>()) return tm;
    return nullptr;
}