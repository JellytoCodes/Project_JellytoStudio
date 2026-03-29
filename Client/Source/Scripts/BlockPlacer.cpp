#include "pch.h"
#include "BlockPlacer.h"

#include "Entity/Entity.h"
#include "Entity/Components/Transform.h"
#include "Entity/Components/MeshRenderer.h"
#include "Entity/Components/TileMap.h"
#include "Entity/Components/Collider/AABBCollider.h"
#include "Graphics/Model/Model.h"
#include "Graphics/Model/ModelRenderer.h"
#include "Core/Managers/InputManager.h"
#include "Scene/Scene.h"
#include "Scene/SceneManager.h"
#include "Scene/SceneSerializer.h"
#include "Resource/Managers/ResourceManager.h"
#include "Resource/Mesh.h"
#include "Resource/Material.h"
#include "Pipeline/Shader.h"

using SlotType    = PaletteWidget::SlotType;
using ColSize     = BlockPlacer::ColliderSize;

// ── 슬롯별 파라미터 정의 ─────────────────────────────────────────────────
//
// collider: 미리 정의된 규격 박스 → Collider + 그리드 배치 기준
// modelScale: 모델 원본 크기를 규격 박스에 맞게 조정하는 시각 스케일
//   → 실측이 어렵기 때문에 여기서 눈으로 보면서 맞추는 값
//
// 규격 박스 크기:
//   Small : 0.5 x 0.5 x 0.5
//   Unit  : 1.0 x 1.0 x 1.0
//   Tall  : 1.0 x 1.5 x 1.0
//   Wide  : 2.0 x 0.5 x 1.0

BlockPlacer::MapModelParams BlockPlacer::GetModelParams(SlotType type) const
{
    switch (type)
    {
    //                                        modelName        collider        modelScale
    case SlotType::Mushroom1: return { L"Mushroom_01", ColSize::Unit,  Vec3(0.012f, 0.012f, 0.012f) };
    case SlotType::Mushroom2: return { L"Mushroom_02", ColSize::Tall,  Vec3(0.012f, 0.012f, 0.012f) };
    case SlotType::Mushroom3: return { L"Mushroom_03", ColSize::Small, Vec3(0.010f, 0.010f, 0.010f) };
    case SlotType::Priming1:  return { L"Priming_01",  ColSize::Tall,  Vec3(0.014f, 0.014f, 0.014f) };
    case SlotType::Priming2:  return { L"Priming_02",  ColSize::Tall,  Vec3(0.014f, 0.014f, 0.014f) };
    case SlotType::Priming3:  return { L"Priming_03",  ColSize::Unit,  Vec3(0.012f, 0.012f, 0.012f) };
    case SlotType::Bridge:    return { L"Bridge",       ColSize::Wide,  Vec3(0.016f, 0.010f, 0.016f) };
    default:                  return { L"",             ColSize::Unit,  Vec3(1.f,  1.f,  1.f)        };
    }
}

// ── 모델 캐시 로드 ────────────────────────────────────────────────────────

std::shared_ptr<Model> BlockPlacer::GetOrLoadModel(SlotType type)
{
    int idx = static_cast<int>(type);
    if (_modelCache[idx]) return _modelCache[idx];

    auto params = GetModelParams(type);
    if (params.modelName.empty()) return nullptr;

    auto model = std::make_shared<Model>();
    model->SetModelPath(L"../Resources/Models/MapModel/");
    model->SetTexturePath(L"../Resources/Textures/MapModel/");
    model->ReadModel(params.modelName);
    model->ReadMaterial(params.modelName);

    _modelCache[idx] = model;

    wchar_t dbg[128];
    swprintf_s(dbg, L"[BlockPlacer] 모델 로드: %s\n", params.modelName.c_str());
    ::OutputDebugStringW(dbg);
    return model;
}

// ── 프리뷰 머티리얼 ───────────────────────────────────────────────────────

std::shared_ptr<Material> BlockPlacer::GetPreviewMat(bool ok)
{
    auto& target = ok ? _previewMatOk : _previewMatBad;
    if (target) return target;

    target = std::make_shared<Material>();
    target->SetShader(std::make_shared<Shader>(L"../Engine/Shaders/MeshShader.hlsl"));
    auto& d = target->GetMaterialDesc();
    d.ambient = d.diffuse = ok ? Vec4(0.2f,0.9f,0.2f,0.5f) : Vec4(0.9f,0.2f,0.2f,0.5f);
    d.specular = d.emissive = Vec4(0,0,0,0);
    return target;
}

// ── 생명주기 ─────────────────────────────────────────────────────────────

BlockPlacer::BlockPlacer() : MonoBehaviour() {}

void BlockPlacer::Awake()
{
    for (int i = 0; i < static_cast<int>(SlotType::Count); i++)
    {
        SlotType t = static_cast<SlotType>(i);
        if (t != SlotType::Eraser)
            GetOrLoadModel(t);
    }
}

void BlockPlacer::OnDestroy() { HidePreview(); }

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

// ── 입력 ─────────────────────────────────────────────────────────────────

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
        auto pal = _palette.lock();
        if (pal && pal->GetSelectedSlotType() == SlotType::Eraser)
            TryRemove(groundPos);
        else
            TryPlace(groundPos);
        return;
    }
    if (input->GetButtonDown(KEY_TYPE::RBUTTON))
    {
        Vec3 gp;
        if (scene->PickGroundPoint((int32)mp.x, (int32)mp.y, gp, 0.f))
            TryRemove(gp);
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

    auto pal     = _palette.lock();
    SlotType st  = pal ? pal->GetSelectedSlotType() : SlotType::Mushroom1;
    bool isErase = (st == SlotType::Eraser);

    bool canAct = false;
    Vec3 previewPos = snapped;
    Vec3 previewScale(1.f);

    if (isErase)
    {
        canAct = IsCellOccupied(col, row);
        previewPos.y = 0.05f;
        previewScale = Vec3(0.9f, 0.05f, 0.9f);
    }
    else
    {
        auto params = GetModelParams(st);
        Vec3 halfExt = GetColliderHalfExtents(params.collider);
        float boxH   = GetColliderFullHeight(params.collider);

        int32 layer; float yPos;
        if (tileMap->IsValid(col, row) && FindNextLayer(col, row, params.collider, layer, yPos))
        {
            // 프리뷰는 콜라이더 박스 크기로 표시
            previewPos.y = yPos + halfExt.y;
            previewScale = Vec3(halfExt.x * 2.f * 0.95f, 0.06f, halfExt.z * 2.f * 0.95f);
            canAct = !IsOverlappingCharacter(Vec3(previewPos.x, yPos + halfExt.y, previewPos.z), halfExt);
        }
    }

    _previewValid = canAct;

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
    _previewEntity->GetTransform()->SetLocalScale(previewScale);

    if (auto mr = _previewEntity->GetComponent<MeshRenderer>())
    {
        mr->SetMesh(GET_SINGLE(ResourceManager)->Get<Mesh>(L"Cube"));
        mr->SetMaterial(GetPreviewMat(canAct));
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

    auto pal      = _palette.lock();
    SlotType type = pal ? pal->GetSelectedSlotType() : SlotType::Mushroom1;
    if (type == SlotType::Eraser) return false;

    auto model = GetOrLoadModel(type);
    if (!model) return false;

    auto params  = GetModelParams(type);
    Vec3 halfExt = GetColliderHalfExtents(params.collider);
    float boxH   = GetColliderFullHeight(params.collider);

    int32 layer; float yPos;
    if (!FindNextLayer(col, row, params.collider, layer, yPos)) return false;

    // Entity 위치: 콜라이더 박스 하단이 yPos에 닿도록 (중심은 yPos + halfExt.y)
    Vec3 center = tileMap->GridToWorld(col, row);
    center.y = yPos + halfExt.y;

    // 캐릭터 겹침 체크
    if (IsOverlappingCharacter(center, halfExt))
    {
        ::OutputDebugStringW(L"[BlockPlacer] 캐릭터 겹침 — 배치 불가\n");
        return false;
    }

    // Entity 생성
    auto blockEntity = std::make_shared<Entity>(L"MapBlock");
    blockEntity->AddComponent(std::make_shared<Transform>());
    blockEntity->GetTransform()->SetLocalPosition(center);
    blockEntity->GetTransform()->SetLocalScale(Vec3(1.f)); // 스케일은 modelScale로 조정

    // ModelRenderer: modelScale을 보정으로 설정
    auto shader = std::make_shared<Shader>(L"../Engine/Shaders/MeshShader.hlsl");
    auto modelRenderer = std::make_shared<ModelRenderer>(shader);
    modelRenderer->SetModel(model);
    modelRenderer->SetModelScale(params.modelScale);
    blockEntity->AddComponent(modelRenderer);

    // AABB Collider: 규격 박스 크기 그대로
    auto col_ = std::make_shared<AABBCollider>();
    col_->SetBoxExtents(halfExt);
    // Entity Transform 중심이 이미 박스 중심이므로 offset 불필요
    blockEntity->AddComponent(col_);

    scene->Add(blockEntity);

    if (layer == 0) tileMap->SetWalkable(col, row, false);

    uint64 key = MakeKey(col, row, layer);
    _blockEntities[key] = blockEntity;
    _placedCells.emplace_back(col, row);

    wchar_t dbg[128];
    swprintf_s(dbg, L"[BlockPlacer] 배치: %s (%d,%d) layer=%d  collider=%s  y=%.2f\n",
        params.modelName.c_str(), col, row, layer,
        (params.collider == ColSize::Small ? L"Small" :
         params.collider == ColSize::Unit  ? L"Unit"  :
         params.collider == ColSize::Tall  ? L"Tall"  : L"Wide"),
        center.y);
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
    for (int32 layer = LAYER_MAX-1; layer >= 0; layer--)
    {
        uint64 key = MakeKey(col, row, layer);
        auto it = _blockEntities.find(key);
        if (it == _blockEntities.end()) continue;

        if (auto scene = GET_SINGLE(SceneManager)->GetCurrentScene())
            scene->Remove(it->second);

        if (layer == 0)
            if (auto tm = FindTileMap()) tm->SetWalkable(col, row, true);

        _blockEntities.erase(it);
        _placedCells.erase(
            std::remove_if(_placedCells.begin(), _placedCells.end(),
                [col,row](const auto& p){ return p.first==col && p.second==row; }),
            _placedCells.end());
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
            int32 c=(int32)(key/1000000), r=(int32)((key%1000000)/100), l=(int32)(key%100);
            if (l == 0) tileMap->SetWalkable(c, r, true);
        }
    }
    _blockEntities.clear();
    _placedCells.clear();
}

// ── 유틸 ─────────────────────────────────────────────────────────────────

bool BlockPlacer::FindNextLayer(int32 col, int32 row, ColliderSize size,
                                int32& outLayer, float& outY) const
{
    float boxH = GetColliderFullHeight(size);
    for (int32 layer = 0; layer < LAYER_MAX; layer++)
    {
        if (!IsCellLayerOccupied(col, row, layer))
        {
            outLayer = layer;
            outY     = boxH * static_cast<float>(layer); // 박스 하단 Y
            return true;
        }
    }
    return false;
}

bool BlockPlacer::IsOverlappingCharacter(const Vec3& center, const Vec3& halfExtents) const
{
    auto charEntity = _character.lock();
    if (!charEntity) return false;
    auto charCol = charEntity->GetComponent<AABBCollider>();
    if (!charCol) return false;

    BoundingBox blockBox;
    blockBox.Center  = center;
    blockBox.Extents = halfExtents;
    return blockBox.Intersects(charCol->GetBoundingBox());
}

bool BlockPlacer::IsCellLayerOccupied(int32 col, int32 row, int32 layer) const
{
    return _blockEntities.count(MakeKey(col,row,layer)) > 0;
}

bool BlockPlacer::IsCellOccupied(int32 col, int32 row) const
{
    return IsCellLayerOccupied(col, row, 0);
}

std::shared_ptr<TileMap> BlockPlacer::FindTileMap() const
{
    auto scene = GET_SINGLE(SceneManager)->GetCurrentScene();
    if (!scene) return nullptr;
    for (auto& e : scene->GetEntities())
        if (auto tm = e->GetComponent<TileMap>()) return tm;
    return nullptr;
}