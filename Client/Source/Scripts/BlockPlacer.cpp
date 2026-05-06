#include "pch.h"
#include "BlockPlacer.h"
#include "UI/InventoryData.h"
#include "Entity/Entity.h"
#include "Entity/Components/Transform.h"
#include "Entity/Components/MeshRenderer.h"
#include "Entity/Components/Collider/AABBCollider.h"
#include "Core/Managers/InputManager.h"
#include "Core/Managers/TimeManager.h"
#include "Scene/Scene.h"
#include "Scene/SceneManager.h"
#include "Scene/SceneSerializer.h"
#include "Scene/ChunkManager.h"
#include "Resource/Managers/ResourceManager.h"
#include "Resource/Mesh.h"
#include "Resource/Material.h"
#include "Pipeline/Shader.h"
#include "Resource/TextureArray.h"
#include "Resource/Texture.h"
#include "Graphics/Managers/InstancingManager.h"
// #include "Audio/AudioManager.h"

using SlotType = PaletteWidget::SlotType;
using CH       = CollisionChannel;
using PF       = PlaceFace;
using CS       = BlockPlacer::ColliderSize;

const AtlasRect BlockPlacer::kAtlasRects[] =
{
    { 0.f, 0.f, 1.f, 1.f },  // Mushroom1
    { 0.f, 0.f, 1.f, 1.f },  // Mushroom2
    { 0.f, 0.f, 1.f, 1.f },  // Mushroom3
    { 0.f, 0.f, 1.f, 1.f },  // Priming1
    { 0.f, 0.f, 1.f, 1.f },  // Priming2
    { 0.f, 0.f, 1.f, 1.f },  // Priming3
    { 0.f, 0.f, 1.f, 1.f },  // Bridge
    { 0.f, 0.f, 1.f, 1.f },  // Eraser (dummy)
};

void BlockPlacer::PushAtlasRects()
{
    if (!_pBlockShader) return;
    auto var = _pBlockShader->GetVector("g_AtlasRects");
    if (var && var->IsValid())
    {
        static_assert(sizeof(AtlasRect) == sizeof(float) * 4,
            "AtlasRect must be 16 bytes (float4)");
        var->SetFloatVectorArray(
            reinterpret_cast<float*>(const_cast<AtlasRect*>(kAtlasRects)),
            0,
            static_cast<uint32>(PaletteWidget::SlotType::Count));
    }
}

Vec3 BlockPlacer::GetHalfExtents(ColliderSize s)
{
    switch (s)
    {
    case CS::Small: return Vec3(0.25f, 0.25f, 0.25f);
    case CS::Unit:  return Vec3(0.5f,  0.5f,  0.5f);
    case CS::Tall:  return Vec3(0.5f,  0.75f, 0.5f);
    case CS::Wide:  return Vec3(1.0f,  0.25f, 0.5f);
    default:        return Vec3(0.5f,  0.5f,  0.5f);
    }
}

BlockPlacer::MapModelParams BlockPlacer::GetModelParams(SlotType type) const
{
    switch (type)
    {
    case SlotType::Mushroom1:
        return { CS::Small, CH::Mushroom,
                 static_cast<uint8>(CH::Priming),
                 static_cast<uint8>(PF::Top) };
    case SlotType::Mushroom2:
        return { CS::Small, CH::Mushroom,
                 static_cast<uint8>(CH::Priming),
                 static_cast<uint8>(PF::Top) };
    case SlotType::Mushroom3:
        return { CS::Small, CH::Mushroom,
                 static_cast<uint8>(CH::Priming),
                 static_cast<uint8>(PF::Top) };
    case SlotType::Priming1:
        return { CS::Unit, CH::Priming,
                 static_cast<uint8>(CH::Priming | CH::Floor | CH::Character),
                 PF::Top | PF::Side };
    case SlotType::Priming2:
        return { CS::Unit, CH::Priming,
                 static_cast<uint8>(CH::Priming | CH::Floor | CH::Character),
                 PF::Top | PF::Side };
    case SlotType::Priming3:
        return { CS::Unit, CH::Priming,
                 static_cast<uint8>(CH::Priming | CH::Floor | CH::Character),
                 PF::Top | PF::Side };
    case SlotType::Bridge:
        return { CS::Wide, CH::Priming,
                 static_cast<uint8>(CH::Priming | CH::Floor | CH::Character),
                 PF::Top | PF::Side };
    default:
        return { CS::Unit, CH::Default,
                 static_cast<uint8>(CH::All),
                 static_cast<uint8>(PF::All) };
    }
}

std::shared_ptr<Material> BlockPlacer::GetPreviewMat(bool ok)
{
    auto& mat = ok ? _previewMatOk : _previewMatBad;
    if (mat) return mat;

    mat = std::make_shared<Material>();
    mat->SetShader(std::make_shared<Shader>(L"../Engine/Shaders/MeshShader.hlsl"));
    auto& d    = mat->GetMaterialDesc();
    d.ambient  = d.diffuse = ok ? Vec4(0.2f, 0.9f, 0.2f, 0.5f)
                                 : Vec4(0.9f, 0.2f, 0.2f, 0.5f);
    d.specular = d.emissive = Vec4(0.f, 0.f, 0.f, 0.f);
    return mat;
}

BlockPlacer::BlockPlacer() : MonoBehaviour() {}

void BlockPlacer::Awake()
{


    _pBlockShader = std::make_shared<Shader>(L"../Engine/Shaders/BlockMeshShader.hlsl");

    _pBlockAtlasTexture = GET_SINGLE(ResourceManager)->Get<Texture>(L"Main_texture");
    if (!_pBlockAtlasTexture)
        _pBlockAtlasTexture = GET_SINGLE(ResourceManager)->Load<Texture>(
            L"Main_texture", L"../Resources/Textures/MapModel/Main_texture.png");

    _pBlockUberMaterial = std::make_shared<Material>();
    _pBlockUberMaterial->SetShader(_pBlockShader);
    _pBlockUberMaterial->SetDiffuseMap(_pBlockAtlasTexture);

    PushAtlasRects();

    auto& desc    = _pBlockUberMaterial->GetMaterialDesc();
    desc.ambient  = Vec4(0.3f, 0.3f, 0.3f, 1.f);
    desc.diffuse  = Vec4(1.f,  1.f,  1.f,  1.f);
    desc.specular = Vec4(0.1f, 0.1f, 0.1f, 1.f);
    desc.emissive = Vec4(0.f,  0.f,  0.f,  0.f);

    _pBlockCubeMesh = GET_SINGLE(ResourceManager)->Get<Mesh>(L"Cube");
    assert(_pBlockCubeMesh && "Cube 메시 로드 실패");
}

void BlockPlacer::Start()
{
    // _pBlockShader 는 Awake 에서 이미 생성됨
}

void BlockPlacer::OnDestroy()
{
    HidePreview();
}

void BlockPlacer::SetPlacingMode(bool on)
{
    _placingMode  = on;
    if (!on) HidePreview();
    if (_palette) _palette->SetPlacingMode(on);
    _previewDirty = true;
}

void BlockPlacer::Update()
{
    const float dt    = GET_SINGLE(TimeManager)->GetDeltaTime();
    auto*       input = GET_SINGLE(InputManager);

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

        scene->PickBlock(mx, my, CH::Priming,
            pick.priming.entity, pick.priming.normal, pick.priming.dist);
        pick.priming.valid = (pick.priming.entity != nullptr);

        scene->PickBlock(mx, my, CH::Floor,
            pick.floor.entity, pick.floor.normal, pick.floor.dist);
        pick.floor.valid = (pick.floor.entity != nullptr);

        scene->PickBlock(mx, my, CH::Mushroom,
            pick.mushroom.entity, pick.mushroom.normal, pick.mushroom.dist);
        pick.mushroom.valid = (pick.mushroom.entity != nullptr);
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
        if      (pick.priming.valid)  TryRemoveEntity(pick.priming.entity);
        else if (pick.mushroom.valid) TryRemoveEntity(pick.mushroom.entity);
        return;
    }

    const auto     params    = GetModelParams(st);
    Entity*        hitEntity = nullptr;
    Vec3           hitNormal;
    float          hitDist   = FLT_MAX;
    bool           hit       = false;

    if (ChannelInMask(CH::Priming, params.pickableMask) && pick.priming.valid)
    { hit = true; hitEntity = pick.priming.entity; hitNormal = pick.priming.normal; hitDist = pick.priming.dist; }

    if (ChannelInMask(CH::Floor, params.pickableMask) && pick.floor.valid)
        if (!hit || pick.floor.dist < hitDist)
        { hit = true; hitEntity = pick.floor.entity; hitNormal = pick.floor.normal; hitDist = pick.floor.dist; }

    if (hit) TryPlaceOnHit(hitEntity, hitNormal, st);
}

bool BlockPlacer::CalcPlacePos(SlotType type, Entity* hitEntity, const Vec3& hitNormal, Vec3& outEntityPos) const
{
    const auto      params  = GetModelParams(type);
    const PlaceFace hitFace = NormalToFace(hitNormal);
    if (!FaceAllowed(hitFace, params.faceMask)) return false;

    const auto* hitAabb = hitEntity->GetComponent<AABBCollider>();
    if (!hitAabb) return false;

    const BoundingBox& hitBox    = const_cast<AABBCollider*>(hitAabb)->GetBoundingBox();
    const Vec3         hitCenter = { hitBox.Center.x,  hitBox.Center.y,  hitBox.Center.z  };
    const Vec3         hitExt    = { hitBox.Extents.x, hitBox.Extents.y, hitBox.Extents.z };
    const Vec3         newHalf   = GetHalfExtents(params.collider);

    Vec3 newColCenter;
    newColCenter.x = hitCenter.x + hitNormal.x * (hitExt.x + newHalf.x);
    newColCenter.y = hitCenter.y + hitNormal.y * (hitExt.y + newHalf.y);
    newColCenter.z = hitCenter.z + hitNormal.z * (hitExt.z + newHalf.z);

    if (IsOverlappingCharacter(newColCenter, newHalf)) return false;

    outEntityPos = newColCenter;
    return true;
}

bool BlockPlacer::IsOverlappingCharacter(const Vec3& colCenter, const Vec3& halfExt) const
{
    if (!_character) return false;
    auto* charCol = _character->GetComponent<AABBCollider>();
    if (!charCol) return false;

    BoundingBox box;
    box.Center  = colCenter;
    box.Extents = halfExt;
    return box.Intersects(charCol->GetBoundingBox());
}

void BlockPlacer::UpdatePreview(const FramePickResult& pick)
{
    Scene* scene = GET_SINGLE(SceneManager)->GetCurrentScene();
    if (!scene) { HidePreview(); return; }

    const SlotType st      = _palette ? _palette->GetSelectedSlotType() : SlotType::Priming1;
    const bool     isErase = (st == SlotType::Eraser);
    const bool     moved   = (pick.mousePos.x != _lastPreviewMouse.x ||
                              pick.mousePos.y != _lastPreviewMouse.y);

    if (!moved && !_previewDirty) return;
    _lastPreviewMouse = pick.mousePos;
    _previewDirty     = false;

    bool canAct     = false;
    Vec3 previewPos = Vec3(0.f);
    Vec3 previewScale;

    if (isErase)
    {
        previewScale = Vec3(1.f, 0.06f, 1.f);
        canAct       = pick.priming.valid || pick.mushroom.valid;
    }
    else
    {
        const auto params   = GetModelParams(st);
        const Vec3 newHalf  = GetHalfExtents(params.collider);
        const bool hasStock = (_pInventory == nullptr || _pInventory->GetCount(st) > 0);

        previewScale = Vec3(newHalf.x * 2.f * 0.95f, 0.06f, newHalf.z * 2.f * 0.95f);

        if (ChannelInMask(CH::Priming, params.pickableMask) && pick.priming.valid)
        {
            Vec3 entityPos;
            if (CalcPlacePos(st, pick.priming.entity, pick.priming.normal, entityPos))
            { canAct = hasStock; previewPos = entityPos; }
        }
        else if (ChannelInMask(CH::Floor, params.pickableMask) && pick.floor.valid)
        {
            Vec3 entityPos;
            if (CalcPlacePos(st, pick.floor.entity, pick.floor.normal, entityPos))
            { canAct = hasStock; previewPos = entityPos; }
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
        mr->SetMesh(_pBlockCubeMesh);
        mr->SetMaterial(GetPreviewMat(canAct));
        instMgr->MarkMeshDirty(mr->GetInstanceID());
    }
}

void BlockPlacer::HidePreview()
{
    if (!_previewEntity) return;
    Scene* scene   = GET_SINGLE(SceneManager)->GetCurrentScene();
    auto*  instMgr = GET_SINGLE(InstancingManager);

    auto* mr = _previewEntity->GetComponent<MeshRenderer>();
    if (mr) instMgr->MarkMeshDirty(mr->GetInstanceID());

    if (scene) scene->Remove(_previewEntity);
    _previewEntity = nullptr;
    _previewValid  = false;
}

Entity* BlockPlacer::SpawnBlockEntity(const Vec3& pos, SlotType type, const Vec3& initialScale, const Vec3& finalScale)
{
    Scene* scene = GET_SINGLE(SceneManager)->GetCurrentScene();
    if (!scene) return nullptr;

    if (!_pBlockUberMaterial || !_pBlockCubeMesh)
    {
        assert(false && "UberMaterial 또는 CubeMesh 미초기화. Awake() 호출 확인.");
        return nullptr;
    }

    const auto params  = GetModelParams(type);
    const Vec3 halfExt = GetHalfExtents(params.collider);

    auto entity = std::make_unique<Entity>(L"MapBlock");
    entity->AddComponent(std::make_unique<Transform>());
    entity->GetComponent<Transform>()->SetLocalPosition(pos);
    entity->GetComponent<Transform>()->SetLocalScale(initialScale);

    auto mr = std::make_unique<MeshRenderer>();
    mr->SetMesh(_pBlockCubeMesh);                   
    mr->SetMaterial(_pBlockUberMaterial);           
    mr->SetMaterialIndex(static_cast<uint32>(type));
    mr->SetPass(0);
    entity->AddComponent(std::move(mr));
    auto col = std::make_unique<AABBCollider>();
    col->SetShowDebug(false);
    col->SetBoxExtents(halfExt);
    col->SetOffsetPosition(Vec3(0.f, 0.f, 0.f));
    col->SetOwnChannel(params.ownChannel);
    col->SetPickableMask(params.pickableMask);
    col->SetStatic(true);
    entity->AddComponent(std::move(col));

    Entity* raw = entity.get();
    scene->Add(std::move(entity));
    return raw;
}

bool BlockPlacer::TryPlaceOnHit(Entity* hitEntity, const Vec3& hitNormal, SlotType type)
{
    Vec3 entityPos;
    if (!CalcPlacePos(type, hitEntity, hitNormal, entityPos)) return false;
    return PlaceBlockAt(entityPos, type);
}

bool BlockPlacer::PlaceBlockAt(const Vec3& entityPos, SlotType type)
{
    if (type == SlotType::Eraser) return false;
    if (_pInventory && !_pInventory->ConsumeItem(type)) return false;

    const auto params     = GetModelParams(type);
    const Vec3 halfExt    = GetHalfExtents(params.collider);
    const Vec3 finalScale = halfExt * 2.f;

    Entity* rawBlock = SpawnBlockEntity(entityPos, type, Vec3(0.001f), finalScale);
    if (!rawBlock) return false;

    auto* mr = rawBlock->GetComponent<MeshRenderer>();
    const InstanceID instID = mr ? mr->GetInstanceID() : InstanceID{ 0, 0 };

    GET_SINGLE(ChunkManager)->Register(rawBlock);
    _blockRecordMap[rawBlock] = { entityPos.x, entityPos.y, entityPos.z,
                                   static_cast<int32>(type) };
    _placedCacheDirty = true;
    _placeTweens.push_back({ rawBlock, 0.f, finalScale });

    if (instID.first != 0)
        GET_SINGLE(InstancingManager)->MarkMeshDirty(instID);

    // GET_SINGLE(AudioManager)->PlayOneShot3D(L"BlockPlace", entityPos);
    return true;
}

bool BlockPlacer::TryRemoveEntity(Entity* entity)
{
    if (!entity) return false;
    auto it = _blockRecordMap.find(entity);
    if (it == _blockRecordMap.end()) return false;

    if (_pInventory)
        _pInventory->AddItem(static_cast<SlotType>(it->second.type), 1);

    auto*            mr         = entity->GetComponent<MeshRenderer>();
    const InstanceID instanceID = mr ? mr->GetInstanceID() : InstanceID{ 0, 0 };

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

    if (instanceID.first != 0)
        GET_SINGLE(InstancingManager)->MarkMeshDirty(instanceID);

    // GET_SINGLE(AudioManager)->PlayOneShot3D(L"BlockRemove", ...);
    return true;
}

bool BlockPlacer::PlaceBlock(float x, float y, float z, int32 typeInt)
{
    if (typeInt < 0 || typeInt >= static_cast<int32>(SlotType::Count)) return false;
    const SlotType type = static_cast<SlotType>(typeInt);
    if (type == SlotType::Eraser) return false;

    const auto params     = GetModelParams(type);
    const Vec3 halfExt    = GetHalfExtents(params.collider);
    const Vec3 finalScale = halfExt * 2.f;

    Entity* rawBlock = SpawnBlockEntity(Vec3(x, y, z), type, finalScale, finalScale);
    if (!rawBlock) return false;

    auto* mr = rawBlock->GetComponent<MeshRenderer>();
    const InstanceID instID = mr ? mr->GetInstanceID() : InstanceID{ 0, 0 };

    GET_SINGLE(ChunkManager)->Register(rawBlock);
    _blockRecordMap[rawBlock] = { x, y, z, typeInt };
    _placedCacheDirty = true;

    if (instID.first != 0)
        GET_SINGLE(InstancingManager)->MarkMeshDirty(instID);

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
                // smoothstep: 3t² - 2t³
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