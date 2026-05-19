#pragma once
#include "Entity/Components/MonoBehaviour.h"
#include "Entity/Components/Collider/CollisionChannel.h"  // BlockPickHit
#include "Scene/BlockPlacerInterface.h"
#include "UI/PaletteWidget.h"
#include "Data/BlockTable.h"

class Mesh;
class Model;
class InventoryData;

class BlockPlacer : public MonoBehaviour, public IBlockPlacer
{
public:
	static Vec3  GetHalfExtents(ColliderSize s);
	static float GetFullHeight(ColliderSize s) { return GetHalfExtents(s).y * 2.f; }

	BlockPlacer();
	virtual ~BlockPlacer() = default;

	virtual void Awake()      override;
	virtual void Start()      override;
	virtual void Update()     override;
	virtual void LateUpdate() override {}
	virtual void OnDestroy()  override;

	void SetPalette(PaletteWidget* p) { _palette = p; }
	void SetSavePath(const std::wstring& path) { _savePath = path; }
	void SetCharacterEntity(Entity* e) { _character = e; }
	void SetInventoryData(InventoryData* inv) { _pInventory = inv; }

	bool IsPlacingMode() const { return _placingMode; }
	void SetPlacingMode(bool on);

	virtual const std::vector<PlacedBlockRecord>& GetPlacedBlocks() const override;
	virtual bool PlaceBlock(float x, float y, float z, int32 type)        override;
	virtual void ClearAllBlocks()                                         override;

	bool PlaceBlockAt(const Vec3& centerPos, PaletteWidget::SlotType type);

private:
	// BlockPickHit(CollisionChannel.h)과 동일 구조였던 Hit을 제거하고
	// 엔진 타입을 직접 사용 — Scene::PickBlocks()에 바로 전달 가능
	struct FramePickResult
	{
		BlockPickHit priming;
		BlockPickHit floor;
		BlockPickHit mushroom;
		POINT        mousePos = { -1, -1 };
	};

	void HandleInput(const FramePickResult& pick);
	void UpdatePreview(const FramePickResult& pick);
	void HidePreview();

	bool TryPlaceOnHit(Entity* hitEntity, const Vec3& hitNormal, PaletteWidget::SlotType type);
	bool TryRemoveEntity(Entity* entity);

	bool CalcPlacePos(PaletteWidget::SlotType type, Entity* hitEntity, const Vec3& hitNormal, Vec3& outCenterPos) const;
	bool ResolvePlaceTarget(const FramePickResult& pick, PaletteWidget::SlotType type, Vec3& outCenterPos) const;
	bool IsOverlappingCharacter(const Vec3& colCenter, const Vec3& halfExt) const;

	Entity* SpawnBlockEntity(const Vec3& centerPos, PaletteWidget::SlotType type, const Vec3& initialScale, const Vec3& finalScale);
	Entity* SpawnMeshBlock(const BlockRecord& rec, const Vec3& centerPos, const Vec3& initialScale, const Vec3& finalScale);
	Entity* SpawnModelBlock(const BlockRecord& rec, const Vec3& centerPos, const Vec3& initialScale, const Vec3& finalScale);

	void AttachCollider(Entity* entity, const BlockRecord& rec);

	struct PlaceTween
	{
		Entity* entity = nullptr;
		float   elapsed = 0.f;
		Vec3    finalScale = Vec3(1.f);
		float   startFrac = 0.15f;
		static constexpr float kDuration = 0.18f;
	};
	void TickPlaceTweens(float dt);

	PaletteWidget* _palette = nullptr;
	Entity* _character = nullptr;
	Entity* _previewEntity = nullptr;
	InventoryData* _pInventory = nullptr;

	bool         _placingMode = false;
	bool         _previewValid = false;
	bool         _previewDirty = true;
	std::wstring _savePath = L"../Saved/scene.json";

	std::shared_ptr<Mesh> _cubeMesh;

	std::unordered_map<std::wstring, std::shared_ptr<Model>> _modelCache;

	POINT _lastPreviewMouse = { -1, -1 };

	std::unordered_map<Entity*, PlacedBlockRecord> _blockRecordMap;
	mutable std::vector<PlacedBlockRecord>         _placedCellsCache;
	mutable bool                                   _placedCacheDirty = true;

	std::vector<PlaceTween> _placeTweens;
};