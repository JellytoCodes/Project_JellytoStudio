#include "pch.h"
#include "BlockPlacer.h"
#include "Data/BlockTable.h"

#include "UI/InventoryData.h"
#include "Entity/Entity.h"
#include "Entity/Components/Transform.h"
#include "Entity/Components/MeshRenderer.h"
#include "Entity/Components/Collider/AABBCollider.h"
#include "Graphics/Model/ModelRenderer.h"
#include "Core/Managers/InputManager.h"
#include "Core/Managers/TimeManager.h"
#include "Scene/Scene.h"
#include "Scene/SceneManager.h"
#include "Scene/SceneSerializer.h"
#include "Scene/ChunkManager.h"
#include "Resource/Managers/ResourceManager.h"
#include "Entity/Components/Camera.h"
#include "Resource/Mesh.h"
#include "Resource/Material.h"
#include "Pipeline/Shader.h"
#include "Graphics/Model/Model.h"
#include "Graphics/Managers/InstancingManager.h"

using SlotType = PaletteWidget::SlotType;
using CH = CollisionChannel;
using PF = PlaceFace;

Vec3 BlockPlacer::GetHalfExtents(ColliderSize s)
{
    switch (s)
    {
    case ColliderSize::Small: return Vec3(0.25f, 0.25f, 0.25f);
    case ColliderSize::Unit:  return Vec3(0.5f, 0.5f, 0.5f);
    case ColliderSize::Tall:  return Vec3(0.5f, 0.75f, 0.5f);
    case ColliderSize::Wide:  return Vec3(1.0f, 0.25f, 0.5f);
    default:                  return Vec3(0.5f, 0.5f, 0.5f);
    }
}

BlockPlacer::BlockPlacer() : MonoBehaviour() {}

void BlockPlacer::Awake()
{
    auto* table = GET_SINGLE(BlockTable);
    assert(table->IsLoaded() && "BlockPlacer::Awake() 전에 BlockTable::Load() 가 선행되어야 합니다.");

    _paletteTex = GET_SINGLE(ResourceManager)->GetOrAddTexture(
        L"BlockPalette", table->GetPalettePath());

    _cubeMesh = GET_SINGLE(ResourceManager)->Get<Mesh>(L"Cube");
    assert(_cubeMesh && "Cube 메시를 찾을 수 없습니다.");

    _meshShader = std::make_shared<Shader>(L"../Engine/Shaders/BlockShader.hlsl");

    _meshUberMaterial = std::make_shared<Material>();
    _meshUberMaterial->SetShader(_meshShader);
    _meshUberMaterial->SetDiffuseMap(_paletteTex);

    auto& desc = _meshUberMaterial->GetMaterialDesc();
    desc.ambient = Vec4(0.3f, 0.3f, 0.3f, 1.f);
    desc.diffuse = Vec4(1.f, 1.f, 1.f, 1.f);
    desc.specular = Vec4(0.1f, 0.1f, 0.1f, 1.f);
    desc.emissive = Vec4(0.f, 0.f, 0.f, 0.f);

    _modelShader = std::make_shared<Shader>(L"../Engine/Shaders/StaticMeshShader.hlsl");

    PushPaletteRects();
}

void BlockPlacer::PushPaletteRects()
{
    if (!_meshShader) return;
    const auto& rects = GET_SINGLE(BlockTable)->GetUVRects();
    if (rects.empty()) return;

    auto var = _meshShader->GetVector("g_AtlasRects");
    if (var && var->IsValid())
    {
        var->SetFloatVectorArray(
            reinterpret_cast<float*>(const_cast<BlockUVRect*>(rects.data())),
            0,
            static_cast<uint32>(rects.size()));
    }
}

void BlockPlacer::Start() {}
void BlockPlacer::OnDestroy() { HidePreview(); }

void BlockPlacer::SetPlacingMode(bool on)
{
    _placingMode = on;
    if (!on) HidePreview();
    if (_palette) _palette->SetPlacingMode(on);
    _previewDirty = true;
}

void BlockPlacer::Update()
{
    const float dt = GET_SINGLE(TimeManager)->GetDeltaTime();
    auto* input = GET_SINGLE(InputManager);

    TickPlaceTweens(dt);

    if (input->GetButtonDown(KEY_TYPE::TAB))
        SetPlacingMode(!_placingMode);

    if ((::GetKeyState(VK_CONTROL) & 0x8000) && input->GetButtonDown(KEY_TYPE::S))
        SceneSerializer::Save(GET_SINGLE(SceneManager)->GetCurrentScene(), _savePath, this);

    if ((::GetKeyState(VK_CONTROL) & 0x8000) && input->GetButtonDown(KEY_TYPE::L))
        SceneSerializer::Load(GET_SINGLE(SceneManager)->GetCurrentScene(), _savePath, this);

    if (!_placingMode) { HidePreview(); return; }

    FramePickResult pick;
    pick.mousePos = input->GetMousePos();

    if (Scene* scene = GET_SINGLE(SceneManager)->GetCurrentScene())
    {
        const int32 mx = static_cast<int32>(pick.mousePos.x);
        const int32 my = static_cast<int32>(pick.mousePos.y);

        BlockPickHit priming;
        BlockPickHit floor;
        BlockPickHit mushroom;
        scene->PickBlocks(mx, my, CH::Priming | CH::Floor | CH::Mushroom,
            priming, floor, mushroom);

        auto assignHit = [](FramePickResult::Hit& dst, const BlockPickHit& src)
        {
            dst.valid  = src.valid;
            dst.entity = src.entity;
            dst.normal = src.normal;
            dst.dist   = src.dist;
        };

        assignHit(pick.priming,  priming);
        assignHit(pick.floor,    floor);
        assignHit(pick.mushroom, mushroom);
    }

    HandleInput(pick);
    UpdatePreview(pick);
}

void BlockPlacer::HandleInput(const FramePickResult& pick)
{
    if (!GET_SINGLE(InputManager)->IsMainWindowActive()) return;
    if (!GET_SINGLE(InputManager)->GetButtonDown(KEY_TYPE::LBUTTON)) return;

    const SlotType st = _palette ? _palette->GetSelectedSlotType() : SlotType::Priming1;

    if (st == SlotType::Eraser)
    {
        if (pick.priming.valid)  TryRemoveEntity(pick.priming.entity);
        else if (pick.mushroom.valid) TryRemoveEntity(pick.mushroom.entity);
        return;
    }

    const BlockRecord* rec = GET_SINGLE(BlockTable)->GetRecord(static_cast<int32>(st));
    if (!rec) return;

    Entity* hitEntity = nullptr;
    Vec3    hitNormal;
    float   hitDist = FLT_MAX;
    bool    hit = false;

    if ((rec->pickableMask & static_cast<uint8>(CH::Priming)) && pick.priming.valid)
    {
        hit = true;
        hitEntity = pick.priming.entity;
        hitNormal = pick.priming.normal;
        hitDist = pick.priming.dist;
    }

    if ((rec->pickableMask & static_cast<uint8>(CH::Floor)) && pick.floor.valid)
    {
        if (!hit || pick.floor.dist < hitDist)
        {
            hit = true;
            hitEntity = pick.floor.entity;
            hitNormal = pick.floor.normal;
            hitDist = pick.floor.dist;
        }
    }

    if (hit) TryPlaceOnHit(hitEntity, hitNormal, st);
}

bool BlockPlacer::CalcPlacePos(SlotType type, Entity* hitEntity,
    const Vec3& hitNormal, Vec3& outCenterPos) const
{
    const BlockRecord* rec = GET_SINGLE(BlockTable)->GetRecord(static_cast<int32>(type));
    if (!rec) return false;

    const PlaceFace hitFace = NormalToFace(hitNormal);
    if (!FaceAllowed(hitFace, rec->faceMask)) return false;

    const auto* hitAabb = hitEntity->GetComponent<AABBCollider>();
    if (!hitAabb) return false;

    const BoundingBox& hitBox = const_cast<AABBCollider*>(hitAabb)->GetBoundingBox();
    const Vec3 hitCenter = { hitBox.Center.x,  hitBox.Center.y,  hitBox.Center.z };
    const Vec3 hitExt = { hitBox.Extents.x, hitBox.Extents.y, hitBox.Extents.z };
    const Vec3 newHalf = GetHalfExtents(rec->collider);

    Vec3 newCenter;
    newCenter.x = hitCenter.x + hitNormal.x * (hitExt.x + newHalf.x);
    newCenter.y = hitCenter.y + hitNormal.y * (hitExt.y + newHalf.y);
    newCenter.z = hitCenter.z + hitNormal.z * (hitExt.z + newHalf.z);

    if (IsOverlappingCharacter(newCenter, newHalf)) return false;

    outCenterPos = newCenter;
    return true;
}

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

void BlockPlacer::UpdatePreview(const FramePickResult& pick)
{
    Scene* scene = GET_SINGLE(SceneManager)->GetCurrentScene();
    if (!scene) { HidePreview(); return; }

    const SlotType st = _palette ? _palette->GetSelectedSlotType() : SlotType::Priming1;
    const bool     isErase = (st == SlotType::Eraser);
    const bool     moved = (pick.mousePos.x != _lastPreviewMouse.x ||
        pick.mousePos.y != _lastPreviewMouse.y);

    if (!moved && !_previewDirty) return;
    _lastPreviewMouse = pick.mousePos;
    _previewDirty = false;

    bool canAct = false;
    Vec3 previewPos;
    Vec3 previewScale;

    if (isErase)
    {
        previewScale = Vec3(1.f, 0.06f, 1.f);
        canAct = pick.priming.valid || pick.mushroom.valid;
    }
    else
    {
        const BlockRecord* rec = GET_SINGLE(BlockTable)->GetRecord(static_cast<int32>(st));
        if (!rec) { HidePreview(); return; }

        const Vec3 half = GetHalfExtents(rec->collider);
        const bool hasStock = (_pInventory == nullptr || _pInventory->GetCount(st) > 0);

        previewScale = Vec3(half.x * 2.f * 0.95f, 0.06f, half.z * 2.f * 0.95f);

        if ((rec->pickableMask & static_cast<uint8>(CH::Priming)) && pick.priming.valid)
        {
            Vec3 cp;
            if (CalcPlacePos(st, pick.priming.entity, pick.priming.normal, cp))
            {
                canAct = hasStock; previewPos = cp;
            }
        }
        else if ((rec->pickableMask & static_cast<uint8>(CH::Floor)) && pick.floor.valid)
        {
            Vec3 cp;
            if (CalcPlacePos(st, pick.floor.entity, pick.floor.normal, cp))
            {
                canAct = hasStock; previewPos = cp;
            }
        }
    }

    _previewValid = canAct;
    auto* instMgr = GET_SINGLE(InstancingManager);

    if (!_previewEntity)
    {
        auto owned = std::make_unique<Entity>(L"__Preview__");
        owned->AddComponent(std::make_unique<Transform>());
        auto mr = std::make_unique<MeshRenderer>();
        mr->SetPass(0);
        owned->AddComponent(std::move(mr));
        _previewEntity = owned.get();
        scene->Add(std::move(owned));
        instMgr->SetMeshDirty();
        return;
    }

    auto* tf = _previewEntity->GetComponent<Transform>();
    tf->SetLocalPosition(previewPos);
    tf->SetLocalScale(previewScale);

    auto* mr = _previewEntity->GetComponent<MeshRenderer>();
    if (mr)
    {
        mr->SetMesh(_cubeMesh);
        mr->SetMaterial(GetPreviewMat(canAct));
        instMgr->MarkMeshDirty(mr->GetInstanceID());
    }
}

void BlockPlacer::HidePreview()
{
    if (!_previewEntity) return;
    Scene* scene = GET_SINGLE(SceneManager)->GetCurrentScene();
    auto* instMgr = GET_SINGLE(InstancingManager);

    if (auto* mr = _previewEntity->GetComponent<MeshRenderer>())
        instMgr->MarkMeshDirty(mr->GetInstanceID());

    if (scene) scene->Remove(_previewEntity);
    _previewEntity = nullptr;
    _previewValid = false;
}

std::shared_ptr<Material> BlockPlacer::GetPreviewMat(bool ok)
{
    auto& mat = ok ? _previewMatOk : _previewMatBad;
    if (mat) return mat;

    mat = std::make_shared<Material>();
    mat->SetShader(std::make_shared<Shader>(L"../Engine/Shaders/StaticMeshShader.hlsl"));
    auto& d = mat->GetMaterialDesc();
    d.ambient = d.diffuse = ok ? Vec4(0.2f, 0.9f, 0.2f, 0.5f)
        : Vec4(0.9f, 0.2f, 0.2f, 0.5f);
    d.specular = d.emissive = Vec4(0.f, 0.f, 0.f, 0.f);
    return mat;
}

void BlockPlacer::AttachCollider(Entity* entity, const BlockRecord& rec)
{
    const Vec3 half = GetHalfExtents(rec.collider);
    auto col = std::make_unique<AABBCollider>();
    col->SetShowDebug(true);
    col->SetBoxExtents(half);
    const float yOffset = (rec.renderType == BlockRenderType::Model) ? half.y : 0.f;
    col->SetOffsetPosition(Vec3(0.f, yOffset, 0.f));
    col->SetOwnChannel(rec.ownChannel);
    col->SetPickableMask(rec.pickableMask);
    col->SetStatic(true);
    entity->AddComponent(std::move(col));
}

Entity* BlockPlacer::SpawnMeshBlock(const BlockRecord& rec, const Vec3& centerPos, const Vec3& initialScale, const Vec3& finalScale)
{
    Scene* scene = GET_SINGLE(SceneManager)->GetCurrentScene();
    if (!scene || !_cubeMesh || !_meshUberMaterial) return nullptr;

    auto entity = std::make_unique<Entity>(L"MapBlock");
    entity->AddComponent(std::make_unique<Transform>());
    auto* tf = entity->GetComponent<Transform>();
    tf->SetLocalPosition(centerPos);
    tf->SetLocalScale(initialScale);

    auto mr = std::make_unique<MeshRenderer>();
    mr->SetMesh(_cubeMesh);
    mr->SetMaterial(_meshUberMaterial);
    mr->SetMaterialIndex(static_cast<uint32>(rec.typeId));
    mr->SetPass(0);
    entity->AddComponent(std::move(mr));

    AttachCollider(entity.get(), rec);

    Entity* raw = entity.get();
    scene->Add(std::move(entity));

    if (auto* col = raw->GetComponent<AABBCollider>())
        col->Update();

    return raw;
}

Entity* BlockPlacer::SpawnModelBlock(const BlockRecord& rec, const Vec3& centerPos, const Vec3& initialScale, const Vec3& finalScale)
{
    Scene* scene = GET_SINGLE(SceneManager)->GetCurrentScene();
    if (!scene || rec.modelName.empty()) return nullptr;

    std::shared_ptr<Model> model;
    const auto cacheIt = _modelCache.find(rec.modelName);
    if (cacheIt != _modelCache.end())
    {
        model = cacheIt->second;
    }
    else
    {
        model = std::make_shared<Model>();
        model->SetModelPath(L"../Resources/Models/MapModel/");
        model->SetTexturePath(L"../Resources/Textures/MapModel/");
        model->ReadModel(rec.modelName);
        model->ReadMaterial(rec.modelName);
        _modelCache[rec.modelName] = model;
    }

    if (!model || model->GetMeshCount() == 0)
    {
        assert(false && "FBX 모델 로드 실패 — BlockMaster.xml 의 modelName 확인");
        return nullptr;
    }

    const Vec3 half = GetHalfExtents(rec.collider);
    const Vec3 bottomPos = Vec3(centerPos.x, centerPos.y - half.y, centerPos.z);

    auto entity = std::make_unique<Entity>(L"MapBlock");
    entity->AddComponent(std::make_unique<Transform>());
    auto* tf = entity->GetComponent<Transform>();
    tf->SetLocalPosition(bottomPos);
    tf->SetLocalScale(initialScale);

    auto modelR = std::make_unique<ModelRenderer>(_modelShader, false);
    modelR->SetModel(model);
    modelR->SetModelScale(Vec3(rec.modelScale));
    entity->AddComponent(std::move(modelR));

    AttachCollider(entity.get(), rec);

    Entity* raw = entity.get();
    scene->Add(std::move(entity));

    if (auto* col = raw->GetComponent<AABBCollider>())
        col->Update();

    return raw;
}

Entity* BlockPlacer::SpawnBlockEntity(const Vec3& centerPos, SlotType type,
    const Vec3& initialScale, const Vec3& finalScale)
{
    const BlockRecord* rec = GET_SINGLE(BlockTable)->GetRecord(static_cast<int32>(type));
    if (!rec)
    {
        assert(false && "BlockRecord 없음 — BlockMaster.xml id 확인");
        return nullptr;
    }

    switch (rec->renderType)
    {
    case BlockRenderType::Mesh:  return SpawnMeshBlock(*rec, centerPos, initialScale, finalScale);
    case BlockRenderType::Model: return SpawnModelBlock(*rec, centerPos, initialScale, finalScale);
    default:                     return nullptr;
    }
}

bool BlockPlacer::TryPlaceOnHit(Entity* hitEntity, const Vec3& hitNormal, SlotType type)
{
    Vec3 centerPos;
    if (!CalcPlacePos(type, hitEntity, hitNormal, centerPos)) return false;
    return PlaceBlockAt(centerPos, type);
}

bool BlockPlacer::PlaceBlockAt(const Vec3& centerPos, SlotType type)
{
    if (type == SlotType::Eraser) return false;
    if (_pInventory && !_pInventory->ConsumeItem(type)) return false;

    const BlockRecord* rec = GET_SINGLE(BlockTable)->GetRecord(static_cast<int32>(type));
    if (!rec) return false;

    const Vec3 half = GetHalfExtents(rec->collider);
    const Vec3 finalScale = half * 2.f;

    Scene* scene = GET_SINGLE(SceneManager)->GetCurrentScene();
    Entity* rawBlock = SpawnBlockEntity(centerPos, type, Vec3(0.001f), finalScale);
    if (!rawBlock) return false;

    GET_SINGLE(ChunkManager)->Register(rawBlock);
    _blockRecordMap[rawBlock] = { centerPos.x, centerPos.y, centerPos.z,
                                   static_cast<int32>(type) };
    _placedCacheDirty = true;
    _placeTweens.push_back({ rawBlock, 0.f, finalScale });

    if (Camera* cam = scene->GetMainCamera())
    {
        cam->SetSortDirty();
        cam->SortEntities();
    }
    GET_SINGLE(InstancingManager)->SetMeshGroupDirty();

    return true;
}

bool BlockPlacer::TryRemoveEntity(Entity* entity)
{
    if (!entity) return false;
    auto it = _blockRecordMap.find(entity);
    if (it == _blockRecordMap.end()) return false;

    if (_pInventory)
        _pInventory->AddItem(static_cast<SlotType>(it->second.type), 1);

    GET_SINGLE(ChunkManager)->Unregister(entity);
    _blockRecordMap.erase(it);
    _placedCacheDirty = true;

    _placeTweens.erase(
        std::remove_if(_placeTweens.begin(), _placeTweens.end(),
            [entity](const PlaceTween& tw) { return tw.entity == entity; }),
        _placeTweens.end());

    Scene* scene = GET_SINGLE(SceneManager)->GetCurrentScene();
    if (!scene) return false;
    scene->Remove(entity);

    if (Camera* cam = scene->GetMainCamera())
    {
        cam->SetSortDirty();
        cam->SortEntities();
    }
    GET_SINGLE(InstancingManager)->SetMeshGroupDirty();

    return true;
}

bool BlockPlacer::PlaceBlock(float x, float y, float z, int32 typeInt)
{
    const BlockRecord* rec = GET_SINGLE(BlockTable)->GetRecord(typeInt);
    if (!rec || static_cast<SlotType>(typeInt) == SlotType::Eraser) return false;

    const Vec3 half = GetHalfExtents(rec->collider);
    const Vec3 finalScale = half * 2.f;

    Entity* rawBlock = SpawnBlockEntity(Vec3(x, y, z), static_cast<SlotType>(typeInt),
        finalScale, finalScale);
    if (!rawBlock) return false;

    GET_SINGLE(ChunkManager)->Register(rawBlock);
    _blockRecordMap[rawBlock] = { x, y, z, typeInt };
    _placedCacheDirty = true;

    GET_SINGLE(InstancingManager)->SetMeshGroupDirty();

    return true;
}

void BlockPlacer::TickPlaceTweens(float dt)
{
    if (_placeTweens.empty()) return;
    auto* instMgr = GET_SINGLE(InstancingManager);

    _placeTweens.erase(
        std::remove_if(_placeTweens.begin(), _placeTweens.end(),
            [&](PlaceTween& tw) -> bool
            {
                if (!tw.entity) return true;
                if (_blockRecordMap.find(tw.entity) == _blockRecordMap.end()) return true;

                tw.elapsed += dt;
                const float t = std::min(tw.elapsed / PlaceTween::kDuration, 1.f);
                const float s = t * t * (3.f - 2.f * t);

                if (auto* tf = tw.entity->GetComponent<Transform>())
                    tf->SetLocalScale(tw.finalScale * s);

                if (auto* col = tw.entity->GetComponent<AABBCollider>())
                    col->InvalidateBounds();

                if (auto* mr = tw.entity->GetComponent<MeshRenderer>())
                    instMgr->MarkMeshDirty(mr->GetInstanceID());

                return t >= 1.f;
            }),
        _placeTweens.end());
}

void BlockPlacer::ClearAllBlocks()
{
    Scene* scene = GET_SINGLE(SceneManager)->GetCurrentScene();
    for (auto& [entity, rec] : _blockRecordMap)
    {
        if (_pInventory)
            _pInventory->AddItem(static_cast<SlotType>(rec.type), 1);
        if (scene) scene->Remove(entity);
    }
    GET_SINGLE(ChunkManager)->Clear();
    _blockRecordMap.clear();
    _placeTweens.clear();
    _placedCacheDirty = true;
    GET_SINGLE(InstancingManager)->SetDirty();
}

const std::vector<PlacedBlockRecord>& BlockPlacer::GetPlacedBlocks() const
{
    if (!_placedCacheDirty) return _placedCellsCache;
    _placedCellsCache.clear();
    _placedCellsCache.reserve(_blockRecordMap.size());
    for (const auto& [entity, rec] : _blockRecordMap)
        _placedCellsCache.push_back(rec);
    _placedCacheDirty = false;
    return _placedCellsCache;
}
