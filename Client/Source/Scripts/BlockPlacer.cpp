#include "pch.h"
#include "BlockPlacer.h"
#include "Data/BlockTable.h"

#include "Resource/BlockMaterialProvider.h"
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
#include "Resource/Mesh.h"
#include "Graphics/Model/Model.h"
#include "Graphics/Managers/InstancingManager.h"

using SlotType = PaletteWidget::SlotType;
using CH       = CollisionChannel;
using PF       = PlaceFace;

Vec3 BlockPlacer::GetHalfExtents(ColliderSize s)
{
	switch (s)
	{
	case ColliderSize::Small: return Vec3(0.25f, 0.25f, 0.25f);
	case ColliderSize::Unit:  return Vec3(0.5f,  0.5f,  0.5f);
	case ColliderSize::Tall:  return Vec3(0.5f,  0.75f, 0.5f);
	case ColliderSize::Wide:  return Vec3(1.0f,  0.25f, 0.5f);
	default:                  return Vec3(0.5f,  0.5f,  0.5f);
	}
}

BlockPlacer::BlockPlacer() : MonoBehaviour() {}

void BlockPlacer::Awake()
{
	auto* table = GET_SINGLE(BlockTable);
	assert(table->IsLoaded() && "BlockPlacer::Awake() 전에 BlockTable::Load() 가 선행되어야 합니다.");

	_cubeMesh = GET_SINGLE(ResourceManager)->Get<Mesh>(L"Cube");
	assert(_cubeMesh && "Cube 메시를 찾을 수 없습니다.");

	GET_SINGLE(BlockMaterialProvider)->Init();
}

void BlockPlacer::Start()     {}
void BlockPlacer::OnDestroy() { HidePreview(); }

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

	const BlockRecord* rec = GET_SINGLE(BlockTable)->GetRecord(static_cast<int32>(st));
	if (!rec) return;

	Vec3 centerPos;
	if (ResolvePlaceTarget(pick, st, centerPos))
		PlaceBlockAt(centerPos, st);
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

	const BoundingBox& hitBox  = const_cast<AABBCollider*>(hitAabb)->GetBoundingBox();
	const Vec3 hitCenter = { hitBox.Center.x,  hitBox.Center.y,  hitBox.Center.z  };
	const Vec3 hitExt    = { hitBox.Extents.x, hitBox.Extents.y, hitBox.Extents.z };
	const Vec3 newHalf   = GetHalfExtents(rec->collider);

	Vec3 newCenter;
	newCenter.x = hitCenter.x + hitNormal.x * (hitExt.x + newHalf.x);
	newCenter.y = hitCenter.y + hitNormal.y * (hitExt.y + newHalf.y);
	newCenter.z = hitCenter.z + hitNormal.z * (hitExt.z + newHalf.z);

	if (IsOverlappingCharacter(newCenter, newHalf)) return false;

	outCenterPos = newCenter;
	return true;
}

bool BlockPlacer::ResolvePlaceTarget(const FramePickResult& pick, SlotType type, Vec3& outCenterPos) const
{
	const BlockRecord* rec = GET_SINGLE(BlockTable)->GetRecord(static_cast<int32>(type));
	if (!rec) return false;

	bool  found    = false;
	float bestDist = FLT_MAX;
	Vec3  bestPos;

	auto tryHit = [&](CollisionChannel channel, const FramePickResult::Hit& hit)
	{
		if (!hit.valid) return;
		if ((rec->pickableMask & static_cast<uint8>(channel)) == 0) return;
		if (hit.dist >= bestDist) return;

		Vec3 pos;
		if (!CalcPlacePos(type, hit.entity, hit.normal, pos)) return;

		found    = true;
		bestDist = hit.dist;
		bestPos  = pos;
	};

	tryHit(CH::Priming,  pick.priming);
	tryHit(CH::Floor,    pick.floor);
	tryHit(CH::Mushroom, pick.mushroom);

	if (!found) return false;
	outCenterPos = bestPos;
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

	bool canAct = false;
	Vec3 previewPos;
	Vec3 previewScale;

	if (isErase)
	{
		previewScale = Vec3(1.f, 0.06f, 1.f);
		canAct       = pick.priming.valid || pick.mushroom.valid;
	}
	else
	{
		const BlockRecord* rec = GET_SINGLE(BlockTable)->GetRecord(static_cast<int32>(st));
		if (!rec) { HidePreview(); return; }

		const Vec3 half     = GetHalfExtents(rec->collider);
		const bool hasStock = (_pInventory == nullptr || _pInventory->GetCount(st) > 0);

		previewScale = Vec3(half.x * 2.f * 0.95f, 0.06f, half.z * 2.f * 0.95f);

		Vec3 cp;
		if (ResolvePlaceTarget(pick, st, cp))
		{ canAct = hasStock; previewPos = cp; }
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
		mr->SetMaterial(GET_SINGLE(BlockMaterialProvider)->GetPreviewMat(canAct));
		instMgr->MarkMeshDirty(mr->GetInstanceID());
	}
}

void BlockPlacer::HidePreview()
{
	if (!_previewEntity) return;
	Scene* scene   = GET_SINGLE(SceneManager)->GetCurrentScene();
	auto*  instMgr = GET_SINGLE(InstancingManager);

	if (auto* mr = _previewEntity->GetComponent<MeshRenderer>())
		instMgr->MarkMeshDirty(mr->GetInstanceID());

	if (scene) scene->Remove(_previewEntity);
	_previewEntity = nullptr;
	_previewValid  = false;
}

void BlockPlacer::AttachCollider(Entity* entity, const BlockRecord& rec)
{
	const Vec3 half = GetHalfExtents(rec.collider);
	auto col = std::make_unique<AABBCollider>();
	col->SetShowDebug(false);
	col->SetBoxExtents(half);
	const float yOffset = (rec.renderType == BlockRenderType::Model) ? half.y : 0.f;
	col->SetOffsetPosition(Vec3(0.f, yOffset, 0.f));
	col->SetOwnChannel(rec.ownChannel);
	col->SetPickableMask(rec.pickableMask);
	col->SetStatic(true);
	entity->AddComponent(std::move(col));
}

Entity* BlockPlacer::SpawnMeshBlock(const BlockRecord& rec, const Vec3& centerPos,
                                     const Vec3& initialScale, const Vec3& finalScale)
{
	Scene* scene = GET_SINGLE(SceneManager)->GetCurrentScene();
	if (!scene || !_cubeMesh) return nullptr;

	auto entity = std::make_unique<Entity>(L"MapBlock");
	entity->AddComponent(std::make_unique<Transform>());
	auto* tf = entity->GetComponent<Transform>();
	tf->SetLocalPosition(centerPos);
	tf->SetLocalScale(initialScale);

	auto mr = std::make_unique<MeshRenderer>();
	mr->SetMesh(_cubeMesh);
	mr->SetMaterial(GET_SINGLE(BlockMaterialProvider)->GetBlockMaterial());
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

Entity* BlockPlacer::SpawnModelBlock(const BlockRecord& rec, const Vec3& centerPos,
                                      const Vec3& initialScale, const Vec3& finalScale)
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
		model->SetModelPath  (L"../Resources/Models/MapModel/");
		model->SetTexturePath(L"../Resources/Textures/MapModel/");
		model->ReadModel   (rec.modelName);
		model->ReadMaterial(rec.modelName);
		_modelCache[rec.modelName] = model;
	}

	if (!model || model->GetMeshCount() == 0)
	{
		assert(false && "FBX 모델 로드 실패 — BlockMaster.json 의 modelName 확인");
		return nullptr;
	}

	const Vec3 half      = GetHalfExtents(rec.collider);
	const Vec3 bottomPos = Vec3(centerPos.x, centerPos.y - half.y, centerPos.z);

	auto entity = std::make_unique<Entity>(L"MapBlock");
	entity->AddComponent(std::make_unique<Transform>());
	auto* tf = entity->GetComponent<Transform>();
	tf->SetLocalPosition(bottomPos);
	tf->SetLocalScale(initialScale);

	auto modelR = std::make_unique<ModelRenderer>(
		GET_SINGLE(BlockMaterialProvider)->GetModelShader(), false);
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
		assert(false && "BlockRecord 없음 — BlockMaster.json id 확인");
		return nullptr;
	}

	switch (rec->renderType)
	{
	case BlockRenderType::Mesh:  return SpawnMeshBlock (*rec, centerPos, initialScale, finalScale);
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

	const Vec3 half       = GetHalfExtents(rec->collider);
	const Vec3 finalScale = half * 2.f;
	const Vec3 initScale  = finalScale * 0.15f;

	Entity* rawBlock = SpawnBlockEntity(centerPos, type, initScale, finalScale);
	if (!rawBlock) return false;

	GET_SINGLE(ChunkManager)->Register(rawBlock);
	_blockRecordMap[rawBlock] = { centerPos.x, centerPos.y, centerPos.z,
	                               static_cast<int32>(type) };
	_placedCacheDirty = true;
	_placeTweens.push_back({ rawBlock, 0.f, finalScale, 0.15f });
	GET_SINGLE(InstancingManager)->SetMeshDirty();

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

	GET_SINGLE(InstancingManager)->SetMeshDirty();

	return true;
}

bool BlockPlacer::PlaceBlock(float x, float y, float z, int32 typeInt)
{
	const BlockRecord* rec = GET_SINGLE(BlockTable)->GetRecord(typeInt);
	if (!rec || static_cast<SlotType>(typeInt) == SlotType::Eraser) return false;

	const Vec3 half       = GetHalfExtents(rec->collider);
	const Vec3 finalScale = half * 2.f;

	Entity* rawBlock = SpawnBlockEntity(Vec3(x, y, z), static_cast<SlotType>(typeInt),
	                                    finalScale, finalScale);
	if (!rawBlock) return false;

	GET_SINGLE(ChunkManager)->Register(rawBlock);
	_blockRecordMap[rawBlock] = { x, y, z, typeInt };
	_placedCacheDirty = true;

	GET_SINGLE(InstancingManager)->SetMeshDirty();

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
				const float t    = std::min(tw.elapsed / PlaceTween::kDuration, 1.f);
				const float ease = t * t * (3.f - 2.f * t);
				const float s    = tw.startFrac + (1.f - tw.startFrac) * ease;

				if (auto* tf = tw.entity->GetComponent<Transform>())
					tf->SetLocalScale(tw.finalScale * s);

				if (auto* col = tw.entity->GetComponent<AABBCollider>())
					col->InvalidateBounds();
				GET_SINGLE(ChunkManager)->MarkDirty(tw.entity);

				if (tw.entity->GetComponent<MeshRenderer>())
					instMgr->SetMeshDirty();
				else if (tw.entity->GetComponent<ModelRenderer>())
					instMgr->SetDirty();

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
