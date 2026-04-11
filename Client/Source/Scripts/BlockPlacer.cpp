
#include "pch.h"
#include "BlockPlacer.h"

#include "UI/InventoryData.h"           // ★ 추가

#include "Entity/Entity.h"
#include "Entity/Components/Transform.h"
#include "Entity/Components/MeshRenderer.h"
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
#include "Graphics/Managers/InstancingManager.h"

using SlotType = PaletteWidget::SlotType;
using CH = CollisionChannel;
using PF = PlaceFace;
using CS = BlockPlacer::ColliderSize;

// ── 정적 헬퍼 ─────────────────────────────────────────────────────────────

Vec3 BlockPlacer::GetHalfExtents(ColliderSize s)
{
    switch (s)
    {
    case CS::Small: return Vec3(0.25f, 0.25f, 0.25f);
    case CS::Unit:  return Vec3(0.5f, 0.5f, 0.5f);
    case CS::Tall:  return Vec3(0.5f, 0.75f, 0.5f);
    case CS::Wide:  return Vec3(1.0f, 0.25f, 0.5f);
    default:        return Vec3(0.5f, 0.5f, 0.5f);
    }
}

BlockPlacer::MapModelParams BlockPlacer::GetModelParams(SlotType type) const
{
    switch (type)
    {
    case SlotType::Mushroom1:
        return { L"Mushroom_01", CS::Small, Vec3(0.01f), CH::Mushroom,
                 static_cast<uint8>(CH::Priming), static_cast<uint8>(PF::Top) };
    case SlotType::Mushroom2:
        return { L"Mushroom_02", CS::Small, Vec3(0.01f), CH::Mushroom,
                 static_cast<uint8>(CH::Priming), static_cast<uint8>(PF::Top) };
    case SlotType::Mushroom3:
        return { L"Mushroom_03", CS::Small, Vec3(0.01f), CH::Mushroom,
                 static_cast<uint8>(CH::Priming), static_cast<uint8>(PF::Top) };
    case SlotType::Priming1:
        return { L"Priming_01", CS::Unit, Vec3(0.01f), CH::Priming,
                 static_cast<uint8>(CH::Priming | CH::Floor | CH::Character),
                 PF::Top | PF::Side };
    case SlotType::Priming2:
        return { L"Priming_02", CS::Unit, Vec3(0.01f), CH::Priming,
                 static_cast<uint8>(CH::Priming | CH::Floor | CH::Character),
                 PF::Top | PF::Side };
    case SlotType::Priming3:
        return { L"Priming_03", CS::Unit, Vec3(0.01f), CH::Priming,
                 static_cast<uint8>(CH::Priming | CH::Floor | CH::Character),
                 PF::Top | PF::Side };
    case SlotType::Bridge:
        return { L"Bridge", CS::Unit, Vec3(0.01f), CH::Priming,
                 static_cast<uint8>(CH::Priming | CH::Floor | CH::Character),
                 PF::Top | PF::Side };
    default:
        return { L"", CS::Unit, Vec3(1.f), CH::Default,
                 static_cast<uint8>(CH::All), static_cast<uint8>(PF::All) };
    }
}

// ── 리소스 캐시 ───────────────────────────────────────────────────────────

std::shared_ptr<Model> BlockPlacer::GetOrLoadModel(SlotType type)
{
    const int idx = static_cast<int>(type);
    if (_modelCache[idx]) return _modelCache[idx];

    const auto params = GetModelParams(type);
    if (params.modelName.empty()) return nullptr;

    auto model = std::make_shared<Model>();
    model->SetModelPath(L"../Resources/Models/MapModel/");
    model->SetTexturePath(L"../Resources/Textures/MapModel/");
    model->ReadModel(params.modelName);
    model->ReadMaterial(params.modelName);

    _modelCache[idx] = model;
    return model;
}

std::shared_ptr<Material> BlockPlacer::GetPreviewMat(bool ok)
{
    auto& mat = ok ? _previewMatOk : _previewMatBad;
    if (mat) return mat;

    mat = std::make_shared<Material>();
    mat->SetShader(std::make_shared<Shader>(L"../Engine/Shaders/MeshShader.hlsl"));
    auto& d = mat->GetMaterialDesc();
    d.ambient = d.diffuse = ok ? Vec4(0.2f, 0.9f, 0.2f, 0.5f)
                               : Vec4(0.9f, 0.2f, 0.2f, 0.5f);
    d.specular = d.emissive = Vec4(0.f, 0.f, 0.f, 0.f);
    return mat;
}

// ── 생명주기 ─────────────────────────────────────────────────────────────

BlockPlacer::BlockPlacer() : MonoBehaviour() {}

void BlockPlacer::Awake()
{
    for (int i = 0; i < static_cast<int>(SlotType::Count); i++)
    {
        const SlotType t = static_cast<SlotType>(i);
        if (t != SlotType::Eraser)
            GetOrLoadModel(t);
    }
}

void BlockPlacer::Start()
{
    if (!_blockShader)
        _blockShader = std::make_shared<Shader>(L"../Engine/Shaders/MeshShader.hlsl");
}

void BlockPlacer::OnDestroy() { HidePreview(); }

// ── 모드 ─────────────────────────────────────────────────────────────────

void BlockPlacer::SetPlacingMode(bool on)
{
    _placingMode = on;
    if (!on) HidePreview();
    if (_palette) _palette->SetPlacingMode(on);
    _previewDirty = true;
}

// ── Update ────────────────────────────────────────────────────────────────

void BlockPlacer::Update()
{
    auto* input = GET_SINGLE(InputManager);

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
    auto* input = GET_SINGLE(InputManager);
    if (!input->IsMainWindowActive()) return;

    Scene* scene = GET_SINGLE(SceneManager)->GetCurrentScene();
    if (!scene) return;

    const POINT mp = input->GetMousePos();
    const SlotType st = _palette ? _palette->GetSelectedSlotType() : SlotType::Priming1;

    if (input->GetButtonDown(KEY_TYPE::LBUTTON))
    {
        if (st == SlotType::Eraser)
        {
            Entity* hitEntity = nullptr;
            Vec3    hitNormal;
            float   hitDist;

            if (scene->PickBlock((int32)mp.x, (int32)mp.y, CH::Priming,
                hitEntity, hitNormal, hitDist))
                TryRemoveEntity(hitEntity);

            else if (scene->PickBlock((int32)mp.x, (int32)mp.y, CH::Mushroom,
                hitEntity, hitNormal, hitDist))
                TryRemoveEntity(hitEntity);
        }
        else
        {
            const auto params = GetModelParams(st);

            Entity* hitEntity = nullptr;
            Vec3    hitNormal;
            float   hitDist = FLT_MAX;
            bool    hit = false;

            if (ChannelInMask(CH::Priming, params.pickableMask))
            {
                Entity* he = nullptr; Vec3 hn; float hd;
                if (scene->PickBlock((int32)mp.x, (int32)mp.y, CH::Priming, he, hn, hd))
                {
                    hit = true; hitEntity = he; hitNormal = hn; hitDist = hd;
                }
            }

            if (ChannelInMask(CH::Floor, params.pickableMask))
            {
                Entity* fe = nullptr; Vec3 fn; float fd;
                if (scene->PickBlock((int32)mp.x, (int32)mp.y, CH::Floor, fe, fn, fd))
                    if (!hit || fd < hitDist)
                    {
                        hit = true; hitEntity = fe; hitNormal = fn; hitDist = fd;
                    }
            }

            if (hit)
                TryPlaceOnHit(hitEntity, hitNormal, st);
        }
    }
}

// ── 배치 위치 계산 ────────────────────────────────────────────────────────

bool BlockPlacer::CalcPlacePos(SlotType type,
    Entity* hitEntity,
    const Vec3& hitNormal,
    Vec3& outEntityPos) const
{
    const auto params = GetModelParams(type);
    const PlaceFace hitFace = NormalToFace(hitNormal);
    if (!FaceAllowed(hitFace, params.faceMask)) return false;

    const auto* hitAabb = hitEntity->GetComponent<AABBCollider>();
    if (!hitAabb) return false;

    const BoundingBox& hitBox = const_cast<AABBCollider*>(hitAabb)->GetBoundingBox();
    const Vec3 hitCenter(hitBox.Center.x, hitBox.Center.y, hitBox.Center.z);
    const Vec3 hitExt(hitBox.Extents.x, hitBox.Extents.y, hitBox.Extents.z);

    const Vec3 newHalf = GetHalfExtents(params.collider);
    Vec3 newColCenter;
    newColCenter.x = hitCenter.x + hitNormal.x * (hitExt.x + newHalf.x);
    newColCenter.y = hitCenter.y + hitNormal.y * (hitExt.y + newHalf.y);
    newColCenter.z = hitCenter.z + hitNormal.z * (hitExt.z + newHalf.z);

    if (IsOverlappingCharacter(newColCenter, newHalf)) return false;

    outEntityPos = Vec3(newColCenter.x, newColCenter.y - newHalf.y, newColCenter.z);
    return true;
}

// ── 프리뷰 ────────────────────────────────────────────────────────────────

void BlockPlacer::UpdatePreview()
{
    Scene* scene = GET_SINGLE(SceneManager)->GetCurrentScene();
    if (!scene) { HidePreview(); return; }

    const POINT mp = GET_SINGLE(InputManager)->GetMousePos();
    const SlotType st = _palette ? _palette->GetSelectedSlotType() : SlotType::Priming1;
    const bool     isErase = (st == SlotType::Eraser);

    const bool mouseMoved = (mp.x != _lastPreviewMouse.x || mp.y != _lastPreviewMouse.y);
    if (!mouseMoved && !_previewDirty) return;

    _lastPreviewMouse = mp;
    _previewDirty = false;

    bool canAct = false;
    Vec3 previewPos = Vec3(0.f, 0.f, 0.f);
    Vec3 previewScale = Vec3(1.f, 0.06f, 1.f);

    if (isErase)
    {
        Entity* hit = nullptr;
        Vec3    hn;
        float   hd;

        canAct = scene->PickBlock((int32)mp.x, (int32)mp.y, CH::Priming, hit, hn, hd)
            || scene->PickBlock((int32)mp.x, (int32)mp.y, CH::Mushroom, hit, hn, hd);

        if (hit)
        {
            AABBCollider* aabb = hit->GetComponent<AABBCollider>();
            const BoundingBox& bb = aabb->GetBoundingBox();
            previewPos = Vec3(bb.Center.x, bb.Center.y + bb.Extents.y, bb.Center.z);
            previewScale = Vec3(bb.Extents.x * 2.f * 0.95f, 0.06f, bb.Extents.z * 2.f * 0.95f);
        }
    }
    else
    {
        // ★ 인벤토리 수량 부족이면 프리뷰도 '불가' 색상으로 표시
        const bool hasStock = !_pInventory || _pInventory->HasItem(st);

        const auto params = GetModelParams(st);
        const Vec3 newHalf = GetHalfExtents(params.collider);

        Entity* hitEntity = nullptr;
        Vec3    hitNormal;
        float   hitDist = FLT_MAX;
        bool    hit = false;

        {
            Entity* he = nullptr; Vec3 hn; float hd;
            if (ChannelInMask(CH::Priming, params.pickableMask) &&
                scene->PickBlock((int32)mp.x, (int32)mp.y, CH::Priming, he, hn, hd))
            {
                hit = true; hitEntity = he; hitNormal = hn; hitDist = hd;
            }
        }
        {
            Entity* he = nullptr; Vec3 hn; float hd;
            if (ChannelInMask(CH::Floor, params.pickableMask) &&
                scene->PickBlock((int32)mp.x, (int32)mp.y, CH::Floor, he, hn, hd))
                if (!hit || hd < hitDist)
                {
                    hit = true; hitEntity = he; hitNormal = hn; hitDist = hd;
                }
        }

        if (hit)
        {
            Vec3 entityPos;
            if (CalcPlacePos(st, hitEntity, hitNormal, entityPos))
            {
                // 위치는 유효하지만 재고가 없으면 빨간 프리뷰
                canAct = hasStock;
                previewPos = entityPos;
                previewScale = Vec3(newHalf.x * 2.f * 0.95f, 0.06f, newHalf.z * 2.f * 0.95f);
            }
        }
    }

    _previewValid = canAct;

    if (!_previewEntity)
    {
        auto owned = std::make_unique<Entity>(L"__Preview__");
        owned->AddComponent(std::make_unique<Transform>());

        auto mr = std::make_unique<MeshRenderer>();
        mr->SetPass(0);
        owned->AddComponent(std::move(mr));

        _previewEntity = owned.get();
        scene->Add(std::move(owned));
    }

    _previewEntity->GetComponent<Transform>()->SetLocalPosition(previewPos);
    _previewEntity->GetComponent<Transform>()->SetLocalScale(previewScale);

    if (auto* mr = _previewEntity->GetComponent<MeshRenderer>())
    {
        mr->SetMesh(GET_SINGLE(ResourceManager)->Get<Mesh>(L"Cube"));
        mr->SetMaterial(GetPreviewMat(canAct));
    }
}

void BlockPlacer::HidePreview()
{
    if (!_previewEntity) return;
    if (Scene* scene = GET_SINGLE(SceneManager)->GetCurrentScene())
        scene->Remove(_previewEntity);
    _previewEntity = nullptr;
    _previewValid = false;
}

// ── 배치 ─────────────────────────────────────────────────────────────────

bool BlockPlacer::TryPlaceOnHit(Entity* hitEntity, const Vec3& hitNormal, SlotType type)
{
    Vec3 entityPos;
    if (!CalcPlacePos(type, hitEntity, hitNormal, entityPos)) return false;
    return PlaceBlockAt(entityPos, type);
}

bool BlockPlacer::PlaceBlockAt(const Vec3& entityPos, SlotType type)
{
    Scene* scene = GET_SINGLE(SceneManager)->GetCurrentScene();
    if (!scene) return false;
    if (type == SlotType::Eraser) return false;

    // ★ 인벤토리 소비 — nullptr이면 무제한(에디터 호환)
    if (_pInventory && !_pInventory->ConsumeItem(type))
        return false; // 수량 부족: 배치 거부

    std::shared_ptr<Model> model = GetOrLoadModel(type);
    if (!model) return false;

    const auto params = GetModelParams(type);
    const Vec3 halfExt = GetHalfExtents(params.collider);

    auto blockEntity = std::make_unique<Entity>(L"MapBlock");
    blockEntity->AddComponent(std::make_unique<Transform>());
    blockEntity->GetComponent<Transform>()->SetLocalPosition(entityPos);
    blockEntity->GetComponent<Transform>()->SetLocalScale(Vec3(1.f));

    auto mr = std::make_unique<ModelRenderer>(_blockShader, false);
    mr->SetModel(model);
    mr->SetModelScale(params.modelScale);
    blockEntity->AddComponent(std::move(mr));

    auto col = std::make_unique<AABBCollider>();
    col->SetShowDebug(false);
    col->SetBoxExtents(halfExt);
    col->SetOffsetPosition(Vec3(0.f, halfExt.y, 0.f));
    col->SetOwnChannel(params.ownChannel);
    col->SetPickableMask(params.pickableMask);
    col->SetStatic(true);
    blockEntity->AddComponent(std::move(col));

    Entity* rawBlock = blockEntity.get();
    scene->Add(std::move(blockEntity));

    _blockSet.insert(rawBlock);
    _blockTypeMap[rawBlock] = type; // ★ 타입 이력 저장

    GET_SINGLE(InstancingManager)->SetDirty();
    return true;
}

// ── 제거 ─────────────────────────────────────────────────────────────────

bool BlockPlacer::TryRemoveEntity(Entity* entity)
{
    if (!entity) return false;
    if (_blockSet.find(entity) == _blockSet.end()) return false;

    // ★ 인벤토리 환급 — 배치했던 타입을 찾아서 돌려줌
    if (_pInventory)
    {
        auto it = _blockTypeMap.find(entity);
        if (it != _blockTypeMap.end())
        {
            _pInventory->AddItem(it->second, 1);
            _blockTypeMap.erase(it);
        }
    }

    if (Scene* scene = GET_SINGLE(SceneManager)->GetCurrentScene())
    {
        scene->Remove(entity);
        _blockSet.erase(entity);
        GET_SINGLE(InstancingManager)->SetDirty();
        return true;
    }
    return false;
}

void BlockPlacer::ClearAllBlocks()
{
    // ★ 전체 제거 시 인벤토리 환급
    if (_pInventory)
    {
        for (auto& [entity, type] : _blockTypeMap)
            _pInventory->AddItem(type, 1);
        _blockTypeMap.clear();
    }

    Scene* scene = GET_SINGLE(SceneManager)->GetCurrentScene();
    for (Entity* e : _blockSet)
        if (scene) scene->Remove(e);
    _blockSet.clear();
    _placedCells.clear();
    GET_SINGLE(InstancingManager)->SetDirty();
}

// ── 유틸 ─────────────────────────────────────────────────────────────────

bool BlockPlacer::IsOverlappingCharacter(const Vec3& colCenter, const Vec3& halfExt) const
{
    if (!_character) return false;

    auto* charCol = _character->GetComponent<AABBCollider>();
    if (!charCol) return false;

    BoundingBox box;
    box.Center = colCenter;
    box.Extents = halfExt;
    return box.Intersects(charCol->GetBoundingBox());
}