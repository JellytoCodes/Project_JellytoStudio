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

	_cubeMesh = GET_SINGLE(ResourceManager)->Get<Mesh>(L"Cube");
	assert(_cubeMesh && "Cube 메시를 찾을 수 없습니다.");

	GET_SINGLE(BlockMaterialProvider)->Init();
	WarmMeshPool();
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

	const SlotType st = _palette ? _palette->GetSelectedSlotType() : SlotType::Priming1;

	if (!_placingMode)
	{
		FramePickResult idlePick;
		idlePick.mousePos = input->GetMousePos();
		UpdatePickDebug(idlePick, st);
		HidePreview();
		return;
	}

	FramePickResult pick;
	pick.mousePos = input->GetMousePos();

	if (Scene* scene = GET_SINGLE(SceneManager)->GetCurrentScene())
	{
		const int32 mx = static_cast<int32>(pick.mousePos.x);
		const int32 my = static_cast<int32>(pick.mousePos.y);

		const uint8 kQueryMask = CH::Priming | CH::Floor | CH::Mushroom;
		scene->PickBlocks(mx, my, kQueryMask, pick.priming, pick.floor, pick.mushroom);
	}

	UpdatePickDebug(pick, st);
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

	auto* hitAabb = hitEntity->GetComponent<AABBCollider>();
	if (!hitAabb) return false;

	const BoundingBox& hitBox = hitAabb->GetBoundingBox();
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

bool BlockPlacer::ResolvePlaceTarget(const FramePickResult& pick, SlotType type, Vec3& outCenterPos) const
{
	const BlockRecord* rec = GET_SINGLE(BlockTable)->GetRecord(static_cast<int32>(type));
	if (!rec) return false;

	bool  found = false;
	float bestDist = FLT_MAX;
	Vec3  bestPos;

	auto tryHit = [&](CollisionChannel channel, const BlockPickHit& hit)
		{
			if (!hit.valid) return;
			if ((rec->pickableMask & static_cast<uint8>(channel)) == 0) return;
			if (hit.dist >= bestDist) return;

			Vec3 pos;
			if (!CalcPlacePos(type, hit.entity, hit.normal, pos)) return;

			found = true;
			bestDist = hit.dist;
			bestPos = pos;
		};

	tryHit(CH::Priming, pick.priming);
	tryHit(CH::Floor, pick.floor);
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
	box.Center = colCenter;
	box.Extents = halfExt;
	return box.Intersects(charCol->GetBoundingBox());
}

void BlockPlacer::UpdatePickDebug(const FramePickResult& pick, SlotType type)
{
	auto FaceToString = [](PlaceFace face) -> std::wstring
		{
			switch (face)
			{
			case PlaceFace::Top:    return L"Top";
			case PlaceFace::Side:   return L"Side";
			case PlaceFace::Bottom: return L"Bottom";
			default:                return L"None";
			}
		};

	auto SlotToString = [](SlotType slot) -> std::wstring
		{
			const BlockRecord* rec = GET_SINGLE(BlockTable)->GetRecord(static_cast<int32>(slot));
			if (rec && !rec->key.empty()) return rec->key;
			return L"Unknown";
		};

	auto CopyHit = [&](const BlockPickHit& src) -> PickDebugInfo::HitInfo
		{
			PickDebugInfo::HitInfo dst;
			dst.valid = src.valid;
			if (!src.valid) return dst;
			dst.entityName = src.entity ? src.entity->GetEntityName() : L"<null>";
			dst.normal = src.normal;
			dst.dist = src.dist;
			dst.face = FaceToString(NormalToFace(src.normal));
			return dst;
		};

	_pickDebug = PickDebugInfo{};
	_pickDebug.placingMode = _placingMode;
	_pickDebug.mousePos = pick.mousePos;
	_pickDebug.eraseMode = (type == SlotType::Eraser);
	_pickDebug.selectedSlot = SlotToString(type);
	_pickDebug.priming = CopyHit(pick.priming);
	_pickDebug.floor = CopyHit(pick.floor);
	_pickDebug.mushroom = CopyHit(pick.mushroom);

	if (!_placingMode)
	{
		_pickDebug.result = L"Idle";
		_pickDebug.rejectReason = L"PlacingMode Off";
		return;
	}

	if (_pickDebug.eraseMode)
	{
		const bool canErase = pick.priming.valid || pick.mushroom.valid;
		_pickDebug.canPlace = canErase;
		_pickDebug.result = canErase ? L"Can Erase" : L"Blocked";
		_pickDebug.rejectReason = canErase ? L"OK" : L"No removable hit";
		_pickDebug.selectedChannel = pick.priming.valid ? L"Priming" : (pick.mushroom.valid ? L"Mushroom" : L"None");
		return;
	}

	const BlockRecord* rec = GET_SINGLE(BlockTable)->GetRecord(static_cast<int32>(type));
	if (!rec)
	{
		_pickDebug.result = L"Blocked";
		_pickDebug.rejectReason = L"Missing BlockRecord";
		return;
	}

	_pickDebug.hasStock = (_pInventory == nullptr || _pInventory->GetCount(type) > 0);
	if (!_pickDebug.hasStock)
	{
		_pickDebug.result = L"Blocked";
		_pickDebug.rejectReason = L"No inventory stock";
		return;
	}

	bool sawHit = false;
	bool sawMaskReject = false;
	bool sawFaceReject = false;
	bool sawCharacterOverlap = false;
	float bestDist = FLT_MAX;

	auto tryHit = [&](CollisionChannel channel, const wchar_t* channelName, const BlockPickHit& hit)
		{
			if (!hit.valid) return;
			sawHit = true;
			if ((rec->pickableMask & static_cast<uint8>(channel)) == 0)
			{
				sawMaskReject = true;
				return;
			}

			const PlaceFace face = NormalToFace(hit.normal);
			if (!FaceAllowed(face, rec->faceMask))
			{
				sawFaceReject = true;
				return;
			}

			if (!hit.entity || !hit.entity->GetComponent<AABBCollider>())
				return;

			const BoundingBox& hitBox = hit.entity->GetComponent<AABBCollider>()->GetBoundingBox();
			const Vec3 hitCenter = { hitBox.Center.x, hitBox.Center.y, hitBox.Center.z };
			const Vec3 hitExt = { hitBox.Extents.x, hitBox.Extents.y, hitBox.Extents.z };
			const Vec3 newHalf = GetHalfExtents(rec->collider);
			const Vec3 newCenter = Vec3(
				hitCenter.x + hit.normal.x * (hitExt.x + newHalf.x),
				hitCenter.y + hit.normal.y * (hitExt.y + newHalf.y),
				hitCenter.z + hit.normal.z * (hitExt.z + newHalf.z));

			if (IsOverlappingCharacter(newCenter, newHalf))
			{
				sawCharacterOverlap = true;
				return;
			}

			if (hit.dist < bestDist)
			{
				bestDist = hit.dist;
				_pickDebug.canPlace = true;
				_pickDebug.resolvedPos = newCenter;
				_pickDebug.selectedChannel = channelName;
			}
		};

	tryHit(CH::Priming, L"Priming", pick.priming);
	tryHit(CH::Floor, L"Floor", pick.floor);
	tryHit(CH::Mushroom, L"Mushroom", pick.mushroom);

	_pickDebug.result = _pickDebug.canPlace ? L"Can Place" : L"Blocked";
	if (_pickDebug.canPlace) _pickDebug.rejectReason = L"OK";
	else if (!sawHit) _pickDebug.rejectReason = L"No hit";
	else if (sawCharacterOverlap) _pickDebug.rejectReason = L"Character overlap";
	else if (sawFaceReject) _pickDebug.rejectReason = L"Face mask reject";
	else if (sawMaskReject) _pickDebug.rejectReason = L"Pickable mask reject";
	else _pickDebug.rejectReason = L"No valid target";
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

		Vec3 cp;
		if (ResolvePlaceTarget(pick, st, cp))
		{
			canAct = hasStock; previewPos = cp;
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
		mr->SetMaterial(GET_SINGLE(BlockMaterialProvider)->GetPreviewMat(canAct));
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

void BlockPlacer::WarmMeshPool()
{
	if (!_cubeMesh) return;
	_meshPool.reserve(kMeshPoolWarmSize);
	for (int32 i = 0; i < kMeshPoolWarmSize; ++i)
	{
		auto entity = std::make_unique<Entity>(L"__PoolBlock__");
		entity->AddComponent(std::make_unique<Transform>());
		auto mr = std::make_unique<MeshRenderer>();
		mr->SetMesh(_cubeMesh);
		mr->SetMaterial(GET_SINGLE(BlockMaterialProvider)->GetBlockMaterial());
		mr->SetPass(0);
		entity->AddComponent(std::move(mr));
		auto col = std::make_unique<AABBCollider>();
		col->SetShowDebug(false);
		col->SetStatic(true);
		entity->AddComponent(std::move(col));
		_meshPool.push_back(std::move(entity));
	}
}

void BlockPlacer::WarmModelPool(const BlockRecord& rec)
{
	if (rec.modelName.empty()) return;
	auto& pool = _modelPools[rec.modelName];
	if (!pool.empty()) return;
	pool.reserve(kModelPoolWarmSize);
	for (int32 i = 0; i < kModelPoolWarmSize; ++i)
	{
		auto entity = std::make_unique<Entity>(L"__PoolBlock__");
		entity->AddComponent(std::make_unique<Transform>());
		auto modelR = std::make_unique<ModelRenderer>(
			GET_SINGLE(BlockMaterialProvider)->GetModelShader(), false);
		entity->AddComponent(std::move(modelR));
		auto col = std::make_unique<AABBCollider>();
		col->SetShowDebug(false);
		col->SetStatic(true);
		entity->AddComponent(std::move(col));
		pool.push_back(std::move(entity));
	}
}

Entity* BlockPlacer::AcquireMeshBlock(const BlockRecord& rec,
	const Vec3& pos, const Vec3& scale)
{
	Scene* scene = GET_SINGLE(SceneManager)->GetCurrentScene();
	if (!scene || !_cubeMesh) return nullptr;

	std::unique_ptr<Entity> entity;
	if (!_meshPool.empty())
	{
		entity = std::move(_meshPool.back());
		_meshPool.pop_back();
	}
	else
	{
		entity = std::make_unique<Entity>(L"__PoolBlock__");
		entity->AddComponent(std::make_unique<Transform>());
		auto mr = std::make_unique<MeshRenderer>();
		mr->SetMesh(_cubeMesh);
		mr->SetMaterial(GET_SINGLE(BlockMaterialProvider)->GetBlockMaterial());
		mr->SetPass(0);
		entity->AddComponent(std::move(mr));
		auto col = std::make_unique<AABBCollider>();
		col->SetShowDebug(false);
		col->SetStatic(true);
		entity->AddComponent(std::move(col));
	}

	auto* tf = entity->GetComponent<Transform>();
	tf->SetLocalPosition(pos);
	tf->SetLocalScale(scale);

	auto* mr = entity->GetComponent<MeshRenderer>();
	mr->SetMaterialIndex(static_cast<uint32>(rec.typeId));

	const Vec3 half = GetHalfExtents(rec.collider);
	auto* col = entity->GetComponent<AABBCollider>();
	col->SetBoxExtents(half);
	col->SetOffsetPosition(Vec3::Zero);
	col->SetOwnChannel(rec.ownChannel);
	col->SetPickableMask(rec.pickableMask);

	Entity* raw = entity.get();
	scene->AddDirect(std::move(entity));
	col->Update();
	return raw;
}

Entity* BlockPlacer::AcquireModelBlock(const BlockRecord& rec,
	const Vec3& pos, const Vec3& scale)
{
	Scene* scene = GET_SINGLE(SceneManager)->GetCurrentScene();
	if (!scene || rec.modelName.empty()) return nullptr;

	auto& pool = _modelPools[rec.modelName];
	std::unique_ptr<Entity> entity;
	if (!pool.empty())
	{
		entity = std::move(pool.back());
		pool.pop_back();
	}
	else
	{
		entity = std::make_unique<Entity>(L"__PoolBlock__");
		entity->AddComponent(std::make_unique<Transform>());
		auto modelR = std::make_unique<ModelRenderer>(
			GET_SINGLE(BlockMaterialProvider)->GetModelShader(), false);
		entity->AddComponent(std::move(modelR));
		auto col = std::make_unique<AABBCollider>();
		col->SetShowDebug(false);
		col->SetStatic(true);
		entity->AddComponent(std::move(col));
	}

	auto* tf = entity->GetComponent<Transform>();
	tf->SetLocalPosition(pos);
	tf->SetLocalScale(scale);

	const auto cacheIt = _modelCache.find(rec.modelName);
	if (cacheIt != _modelCache.end())
	{
		auto* modelR = entity->GetComponent<ModelRenderer>();
		modelR->SetModel(cacheIt->second);
		modelR->SetModelScale(Vec3(rec.modelScale));
	}

	const Vec3 half = GetHalfExtents(rec.collider);
	auto* col = entity->GetComponent<AABBCollider>();
	col->SetBoxExtents(half);
	col->SetOffsetPosition(Vec3(0.f, half.y, 0.f));
	col->SetOwnChannel(rec.ownChannel);
	col->SetPickableMask(rec.pickableMask);

	Entity* raw = entity.get();
	scene->AddDirect(std::move(entity));
	col->Update();
	return raw;
}

void BlockPlacer::ReturnMeshBlock(Entity* entity)
{
	Scene* scene = GET_SINGLE(SceneManager)->GetCurrentScene();
	if (!scene) return;
	std::unique_ptr<Entity> owned = scene->Detach(entity);
	if (!owned) return;
	if (auto* col = owned->GetComponent<AABBCollider>())
		col->SetOwnChannel(CH::None);
	_meshPool.push_back(std::move(owned));
}

void BlockPlacer::ReturnModelBlock(Entity* entity, const std::wstring& modelName)
{
	Scene* scene = GET_SINGLE(SceneManager)->GetCurrentScene();
	if (!scene) return;
	std::unique_ptr<Entity> owned = scene->Detach(entity);
	if (!owned) return;
	if (auto* col = owned->GetComponent<AABBCollider>())
		col->SetOwnChannel(CH::None);
	_modelPools[modelName].push_back(std::move(owned));
}

Entity* BlockPlacer::SpawnMeshBlock(const BlockRecord& rec, const Vec3& centerPos,
	const Vec3& initialScale, const Vec3& finalScale)
{
	return AcquireMeshBlock(rec, centerPos, initialScale);
}

Entity* BlockPlacer::SpawnModelBlock(const BlockRecord& rec, const Vec3& centerPos,
	const Vec3& initialScale, const Vec3& finalScale)
{
	if (_modelCache.find(rec.modelName) == _modelCache.end())
	{
		auto model = std::make_shared<Model>();
		model->SetModelPath(L"../Resources/Models/MapModel/");
		model->SetTexturePath(L"../Resources/Textures/MapModel/");
		model->ReadModel(rec.modelName);
		model->ReadMaterial(rec.modelName);
		if (!model || model->GetMeshCount() == 0)
		{
			assert(false && "FBX model load failed - check modelName in BlockMaster.json");
			return nullptr;
		}
		_modelCache[rec.modelName] = model;
		WarmModelPool(rec);
	}

	const Vec3 half = GetHalfExtents(rec.collider);
	const Vec3 bottomPos = Vec3(centerPos.x, centerPos.y - half.y, centerPos.z);
	return AcquireModelBlock(rec, bottomPos, initialScale);
}

Entity* BlockPlacer::SpawnBlockEntity(const Vec3& centerPos, SlotType type,
	const Vec3& initialScale, const Vec3& finalScale)
{
	const BlockRecord* rec = GET_SINGLE(BlockTable)->GetRecord(static_cast<int32>(type));
	if (!rec)
	{
		assert(false && "Missing BlockRecord - check id in BlockMaster.json");
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
	const Vec3 initScale = finalScale * 0.15f;

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

	const PlacedBlockRecord savedRec = it->second;
	_blockRecordMap.erase(it);
	_placedCacheDirty = true;

	_placeTweens.erase(
		std::remove_if(_placeTweens.begin(), _placeTweens.end(),
			[entity](const PlaceTween& tw) { return tw.entity == entity; }),
		_placeTweens.end());

	const BlockRecord* blockRec = GET_SINGLE(BlockTable)->GetRecord(savedRec.type);
	if (blockRec && blockRec->renderType == BlockRenderType::Mesh)
		ReturnMeshBlock(entity);
	else if (blockRec && blockRec->renderType == BlockRenderType::Model)
		ReturnModelBlock(entity, blockRec->modelName);
	else if (Scene* scene = GET_SINGLE(SceneManager)->GetCurrentScene())
		scene->Remove(entity);

	GET_SINGLE(InstancingManager)->SetMeshDirty();
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
				const float t = std::min(tw.elapsed / PlaceTween::kDuration, 1.f);
				const float ease = t * t * (3.f - 2.f * t);
				const float s = tw.startFrac + (1.f - tw.startFrac) * ease;

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
	for (auto& [entity, rec] : _blockRecordMap)
	{
		if (_pInventory)
			_pInventory->AddItem(static_cast<SlotType>(rec.type), 1);

		GET_SINGLE(ChunkManager)->Unregister(entity);

		const BlockRecord* blockRec = GET_SINGLE(BlockTable)->GetRecord(rec.type);
		if (blockRec && blockRec->renderType == BlockRenderType::Mesh)
			ReturnMeshBlock(entity);
		else if (blockRec && blockRec->renderType == BlockRenderType::Model)
			ReturnModelBlock(entity, blockRec->modelName);
		else if (Scene* scene = GET_SINGLE(SceneManager)->GetCurrentScene())
			scene->Remove(entity);
	}
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
