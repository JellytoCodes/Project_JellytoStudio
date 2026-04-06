#include "pch.h"
#include "BlockPlacer.h"

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
        return { L"Mushroom_01",
                    CS::Small,
                    Vec3(0.01f),
                    CH::Mushroom,
                    static_cast<uint8>(CH::Priming),
                    static_cast<uint8>(PF::Top) };
    case SlotType::Mushroom2:
        return { L"Mushroom_02",
                    CS::Small,
                    Vec3(0.01f),
                    CH::Mushroom,
                    static_cast<uint8>(CH::Priming),
                    static_cast<uint8>(PF::Top) };
    case SlotType::Mushroom3:
        return { L"Mushroom_03",
                    CS::Small,
                    Vec3(0.01f),
                    CH::Mushroom,
                    static_cast<uint8>(CH::Priming),
                    static_cast<uint8>(PF::Top) };
    case SlotType::Priming1:
        return { L"Priming_01",
                    CS::Unit,
                    Vec3(0.01f),
                    CH::Priming,
                    static_cast<uint8>(CH::Priming | CH::Floor | CH::Character),
                    PF::Top | PF::Side };
    case SlotType::Priming2:
        return { L"Priming_02",
                    CS::Unit,
                    Vec3(0.01f),
                    CH::Priming,
                    static_cast<uint8>(CH::Priming | CH::Floor | CH::Character),
                    PF::Top | PF::Side };
    case SlotType::Priming3:
        return { L"Priming_03",
                    CS::Unit,
                    Vec3(0.01f),
                    CH::Priming,
                    static_cast<uint8>(CH::Priming | CH::Floor | CH::Character),
                    PF::Top | PF::Side };
    case SlotType::Bridge:
        return { L"Bridge",
                    CS::Unit,
                    Vec3(0.01f),
                    CH::Priming,
                    static_cast<uint8>(CH::Priming | CH::Floor | CH::Character),
                    PF::Top | PF::Side };
    default:
        return { L"",
                    CS::Unit,
                    Vec3(1.f),
                    CH::Default,
                    static_cast<uint8>(CH::All),
                    static_cast<uint8>(PF::All) };
    }
}

// ── 모델 캐시 ─────────────────────────────────────────────────────────────

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
    return model;
}

// ── 프리뷰 머티리얼 ───────────────────────────────────────────────────────

std::shared_ptr<Material> BlockPlacer::GetPreviewMat(bool ok)
{
    auto& mat = ok ? _previewMatOk : _previewMatBad;
    if (mat) return mat;

    mat = std::make_shared<Material>();
    mat->SetShader(std::make_shared<Shader>(L"../Engine/Shaders/MeshShader.hlsl"));
    auto& d = mat->GetMaterialDesc();
    d.ambient = d.diffuse = ok ? Vec4(0.2f, 0.9f, 0.2f, 0.5f) : Vec4(0.9f, 0.2f, 0.2f, 0.5f);
    d.specular = d.emissive = Vec4(0, 0, 0, 0);
    return mat;
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

    if (!_blockShader)
        _blockShader = std::make_shared<Shader>(L"../Engine/Shaders/MeshShader.hlsl");
}

void BlockPlacer::OnDestroy() { HidePreview(); }

// ── 모드 ─────────────────────────────────────────────────────────────────

void BlockPlacer::SetPlacingMode(bool on)
{
    _placingMode = on;
    if (!on) HidePreview();
    if (auto p = _palette.lock()) p->SetPlacingMode(on);
    _previewDirty = true; // 모드 전환 → 프리뷰 즉시 갱신
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

    auto pal = _palette.lock();
    SlotType st = pal ? pal->GetSelectedSlotType() : SlotType::Priming1;

    if (input->GetButtonDown(KEY_TYPE::LBUTTON))
    {
        if (st == SlotType::Eraser)
        {
            // Eraser: 어떤 채널의 블록이든 제거
            std::shared_ptr<Entity> hitEntity;
            Vec3 hitNormal; float hitDist;
            if (scene->PickBlock((int32)mp.x, (int32)mp.y,
                CH::Priming,     // Priming 채널로 쿼리
                hitEntity, hitNormal, hitDist))
                TryRemoveEntity(hitEntity);
            // Mushroom도 시도
            else if (scene->PickBlock((int32)mp.x, (int32)mp.y,
                CH::Mushroom,
                hitEntity, hitNormal, hitDist))
                TryRemoveEntity(hitEntity);
        }
        else
        {
            auto params = GetModelParams(st);
            // 배치 슬롯의 pickableMask 기준으로 블록 피킹
            // pickableMask에 있는 채널 중 하나로 쿼리
            std::shared_ptr<Entity> hitEntity;
            Vec3 hitNormal; float hitDist;

            // Floor 포함 여부 확인: Floor 채널 블록(Ground)을 먼저 시도
            bool hit = false;

            // Priming 채널 피킹 시도
            if (ChannelInMask(CH::Priming, params.pickableMask))
                hit = scene->PickBlock((int32)mp.x, (int32)mp.y,
                    CH::Priming, hitEntity, hitNormal, hitDist);

            // Floor 채널 피킹 시도 (더 가까우면 교체)
            if (ChannelInMask(CH::Floor, params.pickableMask))
            {
                std::shared_ptr<Entity> floorHit; Vec3 fn; float fd;
                if (scene->PickBlock((int32)mp.x, (int32)mp.y,
                    CH::Floor, floorHit, fn, fd))
                {
                    if (!hit || fd < hitDist)
                    {
                        hit = true;
                        hitEntity = floorHit;
                        hitNormal = fn;
                        hitDist = fd;
                    }
                }
            }

            if (hit)
                TryPlaceOnHit(hitEntity, hitNormal, st);
        }
    }
}

// ── 배치 위치 계산 ────────────────────────────────────────────────────────

bool BlockPlacer::CalcPlacePos(SlotType type,
    const std::shared_ptr<Entity>& hitEntity,
    const Vec3& hitNormal,
    Vec3& outEntityPos) const
{
    auto params = GetModelParams(type);

    // 면 허용 체크
    PlaceFace hitFace = NormalToFace(hitNormal);
    if (!FaceAllowed(hitFace, params.faceMask)) return false;

    // 피킹된 블록의 BoundingBox
    auto hitAabb = hitEntity->GetComponent<AABBCollider>();
    if (!hitAabb) return false;

    const BoundingBox& hitBox = hitAabb->GetBoundingBox();
    Vec3 hitCenter(hitBox.Center.x, hitBox.Center.y, hitBox.Center.z);
    Vec3 hitExt(hitBox.Extents.x, hitBox.Extents.y, hitBox.Extents.z);

    // 새 블록 half-extents
    Vec3 newHalf = GetHalfExtents(params.collider);

    // 새 블록 Collider 중심 = 히트 박스 중심 + 노말 방향 × (각 축 extents 합)
    Vec3 newColCenter;
    newColCenter.x = hitCenter.x + hitNormal.x * (hitExt.x + newHalf.x);
    newColCenter.y = hitCenter.y + hitNormal.y * (hitExt.y + newHalf.y);
    newColCenter.z = hitCenter.z + hitNormal.z * (hitExt.z + newHalf.z);

    // 캐릭터 겹침 체크
    if (IsOverlappingCharacter(newColCenter, newHalf)) return false;

    // Entity 위치 = Collider 중심 - (0, halfExt.y, 0)  (하단 기준)
    outEntityPos = Vec3(newColCenter.x, newColCenter.y - newHalf.y, newColCenter.z);
    return true;
}

// ── 프리뷰 ────────────────────────────────────────────────────────────────

void BlockPlacer::UpdatePreview()
{
    auto scene = GET_SINGLE(SceneManager)->GetCurrentScene();
    if (!scene) { HidePreview(); return; }

    POINT mp = GET_SINGLE(InputManager)->GetMousePos();

    // ── 마우스 델타 캐시 ──────────────────────────────────────────
    // 마우스가 안 움직이면 PickBlock(O(N)) 재실행 스킵
    // 슬롯 변경 시에는 _previewDirty = true로 강제 갱신
    auto pal = _palette.lock();
    SlotType st = pal ? pal->GetSelectedSlotType() : SlotType::Priming1;
    bool isErase = (st == SlotType::Eraser);

    bool mouseMoved = (mp.x != _lastPreviewMouse.x || mp.y != _lastPreviewMouse.y);
    if (!mouseMoved && !_previewDirty)
    {
        // 프리뷰 엔티티는 이미 씬에 있으므로 위치 업데이트만 유지
        return;
    }
    _lastPreviewMouse = mp;
    _previewDirty = false;


    bool canAct = false;
    Vec3 previewPos(0, 0, 0);
    Vec3 previewScale(1, 0.06f, 1);

    if (isErase)
    {
        std::shared_ptr<Entity> hit; Vec3 hn; float hd;
        canAct = scene->PickBlock((int32)mp.x, (int32)mp.y,
            CH::Priming, hit, hn, hd) ||
            scene->PickBlock((int32)mp.x, (int32)mp.y,
                CH::Mushroom, hit, hn, hd);
        if (hit)
        {
            auto aabb = hit->GetComponent<AABBCollider>();
            auto& bb = aabb->GetBoundingBox();
            previewPos = Vec3(bb.Center.x, bb.Center.y + bb.Extents.y, bb.Center.z);
            previewScale = Vec3(bb.Extents.x * 2.f * 0.95f, 0.06f, bb.Extents.z * 2.f * 0.95f);
        }
    }
    else
    {
        auto params = GetModelParams(st);
        Vec3 newHalf = GetHalfExtents(params.collider);

        // 피킹 시도 (Priming 먼저, Floor 다음)
        std::shared_ptr<Entity> hitEntity; Vec3 hitNormal; float hitDist = FLT_MAX;
        bool hit = false;

        {
            std::shared_ptr<Entity> he; Vec3 hn; float hd;
            if (ChannelInMask(CH::Priming, params.pickableMask) &&
                scene->PickBlock((int32)mp.x, (int32)mp.y, CH::Priming, he, hn, hd))
            {
                hit = true; hitEntity = he; hitNormal = hn; hitDist = hd;
            }
        }
        {
            std::shared_ptr<Entity> he; Vec3 hn; float hd;
            if (ChannelInMask(CH::Floor, params.pickableMask) &&
                scene->PickBlock((int32)mp.x, (int32)mp.y, CH::Floor, he, hn, hd))
            {
                if (!hit || hd < hitDist)
                {
                    hit = true; hitEntity = he; hitNormal = hn; hitDist = hd;
                }
            }
        }

        if (hit)
        {
            Vec3 entityPos;
            if (CalcPlacePos(st, hitEntity, hitNormal, entityPos))
            {
                canAct = true;
                previewPos = entityPos; // 박스 하단
                previewScale = Vec3(newHalf.x * 2.f * 0.95f, 0.06f, newHalf.z * 2.f * 0.95f);
            }
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

bool BlockPlacer::TryPlaceOnHit(const std::shared_ptr<Entity>& hitEntity,
    const Vec3& hitNormal,
    SlotType type)
{
    Vec3 entityPos;
    if (!CalcPlacePos(type, hitEntity, hitNormal, entityPos)) return false;
    return PlaceBlockAt(entityPos, type);
}

bool BlockPlacer::PlaceBlockAt(const Vec3& entityPos, SlotType type)
{
    auto scene = GET_SINGLE(SceneManager)->GetCurrentScene();
    if (!scene) return false;

    if (type == SlotType::Eraser) return false;

    auto model = GetOrLoadModel(type);
    if (!model) return false;

    auto params = GetModelParams(type);
    Vec3 halfExt = GetHalfExtents(params.collider);

    // Entity 생성
    auto blockEntity = std::make_shared<Entity>(L"MapBlock");
    blockEntity->AddComponent(std::make_shared<Transform>());
    blockEntity->GetTransform()->SetLocalPosition(entityPos);
    blockEntity->GetTransform()->SetLocalScale(Vec3(1.f));

    auto mr = std::make_shared<ModelRenderer>(_blockShader, false);
    mr->SetModel(model);
    mr->SetModelScale(params.modelScale);
    blockEntity->AddComponent(mr);

    // AABBCollider — 채널 설정
    auto col = std::make_shared<AABBCollider>();
    col->SetShowDebug(false);
    col->SetBoxExtents(halfExt);
    col->SetOffsetPosition(Vec3(0.f, halfExt.y, 0.f));
    col->SetOwnChannel(params.ownChannel);
    col->SetPickableMask(params.pickableMask);
    // 배치 블록은 정적 — 최초 1회만 행렬 연산, 이후 스킵
    col->SetStatic(true);
    blockEntity->AddComponent(col);

    scene->Add(blockEntity);
    _blockSet.insert(blockEntity);

    GET_SINGLE(InstancingManager)->SetDirty();

    return true;
}

// ── 제거 ─────────────────────────────────────────────────────────────────

bool BlockPlacer::TryRemoveEntity(const std::shared_ptr<Entity>& entity)
{
    if (!entity) return false;
    if (_blockSet.find(entity) == _blockSet.end()) return false; // 우리가 배치한 블록만

    if (auto scene = GET_SINGLE(SceneManager)->GetCurrentScene())
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
    auto scene = GET_SINGLE(SceneManager)->GetCurrentScene();
    for (auto& e : _blockSet)
        if (scene) scene->Remove(e);
    _blockSet.clear();
    _placedCells.clear();

    GET_SINGLE(InstancingManager)->SetDirty();
}

// ── 유틸 ─────────────────────────────────────────────────────────────────

bool BlockPlacer::IsOverlappingCharacter(const Vec3& colCenter, const Vec3& halfExt) const
{
    auto charEntity = _character.lock();
    if (!charEntity) return false;
    auto charCol = charEntity->GetComponent<AABBCollider>();
    if (!charCol) return false;

    BoundingBox box;
    box.Center = colCenter;
    box.Extents = halfExt;
    return box.Intersects(charCol->GetBoundingBox());
}