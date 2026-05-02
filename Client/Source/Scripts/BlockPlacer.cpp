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
#include "Graphics/Graphics.h"
#include "Graphics/Managers/InstancingManager.h"
// #include "Audio/AudioManager.h"

using SlotType = PaletteWidget::SlotType;
using CH       = CollisionChannel;
using PF       = PlaceFace;
using CS       = BlockPlacer::ColliderSize;

static const wchar_t* kBlockTexturePaths[] =
{
    L"../Resources/Textures/MapModel/Main_texture.png",  // SlotType::Mushroom1 = 0
    L"../Resources/Textures/MapModel/Main_texture.png",  // SlotType::Mushroom2 = 1
    L"../Resources/Textures/MapModel/Main_texture.png",  // SlotType::Mushroom3 = 2
    L"../Resources/Textures/MapModel/Main_texture.png",   // SlotType::Priming1  = 3
    L"../Resources/Textures/MapModel/Main_texture.png",   // SlotType::Priming2  = 4
    L"../Resources/Textures/MapModel/Main_texture.png",   // SlotType::Priming3  = 5
    L"../Resources/Textures/MapModel/Main_texture.png",       // SlotType::Bridge    = 6
    L"../Resources/Textures/MapModel/Main_texture.png",   // SlotType::Eraser    = 7 (더미)
};
static_assert(
    _countof(kBlockTexturePaths) == static_cast<uint32>(SlotType::Count),
    "kBlockTexturePaths 항목 수가 SlotType::Count 와 일치해야 합니다.");

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

ComPtr<ID3D11ShaderResourceView> BlockPlacer::BuildBlockTextureArray(ID3D11Device* device, ID3D11DeviceContext* ctx) const
{
    constexpr uint32 kSliceCount = static_cast<uint32>(SlotType::Count);

    ComPtr<ID3D11Texture2D> sliceTextures[kSliceCount];
    D3D11_TEXTURE2D_DESC    slice0Desc = {};

    for (uint32 i = 0; i < kSliceCount; ++i)
    {
        ComPtr<ID3D11Resource> res;

        DirectX::ScratchImage image;
        HRESULT hr = S_OK;

        hr = DirectX::LoadFromWICFile(
            kBlockTexturePaths[i], 
            DirectX::WIC_FLAGS_NONE, 
            nullptr, 
            image
        );

        if (SUCCEEDED(hr))
        {
            hr = DirectX::CreateTexture(
                device,
                image.GetImages(),
                image.GetImageCount(),
                image.GetMetadata(),
                res.GetAddressOf()
            );
        }

        if (FAILED(hr))
        {
            sliceTextures[i] = sliceTextures[max(0u, i - 1u)];
            assert(sliceTextures[i] != nullptr && "첫 번째 텍스처 로드 실패");
            continue;
        }

        hr = res.As(&sliceTextures[i]);
        assert(SUCCEEDED(hr));

        if (i == 0)
            sliceTextures[0]->GetDesc(&slice0Desc);
    }

    D3D11_TEXTURE2D_DESC arrayDesc   = slice0Desc;
    arrayDesc.ArraySize              = kSliceCount;
    arrayDesc.Usage                  = D3D11_USAGE_DEFAULT;
    arrayDesc.BindFlags              = D3D11_BIND_SHADER_RESOURCE;
    arrayDesc.CPUAccessFlags         = 0;
    arrayDesc.MiscFlags              = 0;

    ComPtr<ID3D11Texture2D> textureArray;
    HRESULT hr = device->CreateTexture2D(&arrayDesc, nullptr,
                                          textureArray.GetAddressOf());
    if (FAILED(hr))
    {
        assert(false && "Texture2DArray 생성 실패. 텍스처 해상도/포맷 일치 확인.");
        return nullptr;
    }

    for (uint32 i = 0; i < kSliceCount; ++i)
    {
        for (uint32 mip = 0; mip < slice0Desc.MipLevels; ++mip)
        {
            ctx->CopySubresourceRegion(
                textureArray.Get(),
                D3D11CalcSubresource(mip, i, slice0Desc.MipLevels),
                0, 0, 0,                  
                sliceTextures[i].Get(),
                D3D11CalcSubresource(mip, 0, slice0Desc.MipLevels),
                nullptr);                  
        }
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc         = {};
    srvDesc.Format                                  = arrayDesc.Format;
    srvDesc.ViewDimension                           = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
    srvDesc.Texture2DArray.MostDetailedMip          = 0;
    srvDesc.Texture2DArray.MipLevels                = arrayDesc.MipLevels;
    srvDesc.Texture2DArray.FirstArraySlice          = 0;
    srvDesc.Texture2DArray.ArraySize                = kSliceCount;

    ComPtr<ID3D11ShaderResourceView> srv;
    hr = device->CreateShaderResourceView(textureArray.Get(), &srvDesc,
                                           srv.GetAddressOf());
    assert(SUCCEEDED(hr) && "Texture2DArray SRV 생성 실패.");

    return srv;
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
    auto* graphics = GET_SINGLE(Graphics);
    auto* device   = graphics->GetDevice().Get();
    auto* ctx      = graphics->GetDeviceContext().Get();

    _pBlockShader = std::make_shared<Shader>(L"../Engine/Shaders/BlockMeshShader.hlsl");

    _pBlockTextureArray = BuildBlockTextureArray(device, ctx);

    _pBlockUberMaterial = std::make_shared<Material>();
    _pBlockUberMaterial->SetShader(_pBlockShader);
    _pBlockUberMaterial->SetTextureArray(_pBlockTextureArray);

    auto& desc    = _pBlockUberMaterial->GetMaterialDesc();
    desc.ambient  = Vec4(0.3f, 0.3f, 0.3f, 1.f);
    desc.diffuse  = Vec4(1.f,  1.f,  1.f,  1.f);
    desc.specular = Vec4(0.1f, 0.1f, 0.1f, 1.f);
    desc.emissive = Vec4(0.f,  0.f,  0.f,  0.f);

    _pBlockCubeMesh = GET_SINGLE(ResourceManager)->Get<Mesh>(L"Cube");
    assert(_pBlockCubeMesh != nullptr && "Cube 메시 로드 실패.");
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

    HandleInput();
    UpdatePreview();
}

void BlockPlacer::HandleInput()
{
    auto*  input = GET_SINGLE(InputManager);
    if (!input->IsMainWindowActive()) return;

    Scene* scene = GET_SINGLE(SceneManager)->GetCurrentScene();
    if (!scene) return;

    const POINT    mp = input->GetMousePos();
    const SlotType st = _palette ? _palette->GetSelectedSlotType() : SlotType::Priming1;

    if (!input->GetButtonDown(KEY_TYPE::LBUTTON)) return;

    if (st == SlotType::Eraser)
    {
        Entity* hitEntity = nullptr;
        Vec3    hitNormal;
        float   hitDist;
        if (scene->PickBlock((int32)mp.x, (int32)mp.y, CH::Priming, hitEntity, hitNormal, hitDist))
            TryRemoveEntity(hitEntity);
        else if (scene->PickBlock((int32)mp.x, (int32)mp.y, CH::Mushroom, hitEntity, hitNormal, hitDist))
            TryRemoveEntity(hitEntity);
        return;
    }

    const auto params    = GetModelParams(st);
    Entity*    hitEntity = nullptr;
    Vec3       hitNormal;
    float      hitDist   = FLT_MAX;
    bool       hit       = false;

    if (ChannelInMask(CH::Priming, params.pickableMask))
    {
        Entity* he = nullptr; Vec3 hn; float hd;
        if (scene->PickBlock((int32)mp.x, (int32)mp.y, CH::Priming, he, hn, hd))
        { hit = true; hitEntity = he; hitNormal = hn; hitDist = hd; }
    }
    if (ChannelInMask(CH::Floor, params.pickableMask))
    {
        Entity* fe = nullptr; Vec3 fn; float fd;
        if (scene->PickBlock((int32)mp.x, (int32)mp.y, CH::Floor, fe, fn, fd))
            if (!hit || fd < hitDist)
            { hit = true; hitEntity = fe; hitNormal = fn; hitDist = fd; }
    }

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

    outEntityPos = Vec3(newColCenter.x, newColCenter.y - newHalf.y, newColCenter.z);
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

void BlockPlacer::UpdatePreview()
{
    Scene* scene = GET_SINGLE(SceneManager)->GetCurrentScene();
    if (!scene) { HidePreview(); return; }

    const POINT    mp       = GET_SINGLE(InputManager)->GetMousePos();
    const SlotType st       = _palette ? _palette->GetSelectedSlotType() : SlotType::Priming1;
    const bool     isErase  = (st == SlotType::Eraser);
    const bool     moved    = (mp.x != _lastPreviewMouse.x || mp.y != _lastPreviewMouse.y);

    if (!moved && !_previewDirty) return;
    _lastPreviewMouse = mp;
    _previewDirty     = false;

    bool canAct      = false;
    Vec3 previewPos  = Vec3(0.f);
    Vec3 previewScale;

    if (isErase)
    {
        previewScale = Vec3(1.f, 0.06f, 1.f);
        Entity* hit = nullptr; Vec3 hn; float hd;
        canAct = scene->PickBlock((int32)mp.x, (int32)mp.y, CH::Priming, hit, hn, hd)
              || scene->PickBlock((int32)mp.x, (int32)mp.y, CH::Mushroom, hit, hn, hd);
    }
    else
    {
        const auto params   = GetModelParams(st);
        const Vec3 newHalf  = GetHalfExtents(params.collider);
        const bool hasStock = (_pInventory == nullptr || _pInventory->GetCount(st) > 0);

        previewScale = Vec3(newHalf.x * 2.f * 0.95f, 0.06f, newHalf.z * 2.f * 0.95f);

        Entity* hitEntity = nullptr; Vec3 hn; float hd;
        if (ChannelInMask(CH::Priming, params.pickableMask) &&
            scene->PickBlock((int32)mp.x, (int32)mp.y, CH::Priming, hitEntity, hn, hd))
        {
            Vec3 entityPos;
            if (CalcPlacePos(st, hitEntity, hn, entityPos))
            { canAct = hasStock; previewPos = entityPos; }
        }
        else if (ChannelInMask(CH::Floor, params.pickableMask))
        {
            Entity* fe = nullptr; Vec3 fn; float fd;
            if (scene->PickBlock((int32)mp.x, (int32)mp.y, CH::Floor, fe, fn, fd))
            {
                Vec3 entityPos;
                if (CalcPlacePos(st, fe, fn, entityPos))
                { canAct = hasStock; previewPos = entityPos; }
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
    col->SetOffsetPosition(Vec3(0.f, halfExt.y, 0.f));
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