#pragma once
#include "Entity/Components/MonoBehaviour.h"
#include "Scene/BlockPlacerInterface.h"
#include "Entity/Components/Collider/CollisionChannel.h"
#include "UI/PaletteWidget.h"

class Material;
class Model;
class Shader;

class BlockPlacer : public MonoBehaviour, public IBlockPlacer
{
public:
	enum class ColliderSize : uint8
	{
		Small, Unit, Tall, Wide,
	};
	static Vec3  GetHalfExtents(ColliderSize s);
	static float GetFullHeight(ColliderSize s) { return GetHalfExtents(s).y * 2.f; }

	BlockPlacer();
	virtual ~BlockPlacer() = default;

	virtual void Awake()      override;
	virtual void Start()      override;
	virtual void Update()     override;
	virtual void LateUpdate() override {}
	virtual void OnDestroy()  override;

	// 이전: shared_ptr<PaletteWidget> / shared_ptr<Entity>
	// 변경: raw pointer — 씬이 수명 관리, BlockPlacer는 관찰자
	void SetPalette(PaletteWidget* palette)   { _palette   = palette; }
	void SetSavePath(const std::wstring& path){ _savePath  = path; }
	void SetCharacterEntity(Entity* character){ _character = character; }

	bool IsPlacingMode() const { return _placingMode; }
	void SetPlacingMode(bool on);

	// IBlockPlacer
	virtual const std::vector<std::pair<int32, int32>>& GetPlacedBlocks() const override { return _placedCells; }
	virtual bool PlaceBlock(int32 col, int32 row) override { return false; }
	virtual void ClearAllBlocks() override;

	bool PlaceBlockAt(const Vec3& entityPos, PaletteWidget::SlotType type);

private:
	struct MapModelParams
	{
		std::wstring     modelName;
		ColliderSize     collider;
		Vec3             modelScale;
		CollisionChannel ownChannel;
		uint8            pickableMask;
		uint8            faceMask;
	};

	MapModelParams GetModelParams(PaletteWidget::SlotType type) const;

	// 이전: shared_ptr<Model/Material> 반환 → 매 호출마다 refcount 증감
	// 변경: Model*/Material* — ResourceManager 또는 로컬 캐시가 소유
	Model*    GetOrLoadModel(PaletteWidget::SlotType type);
	Material* GetPreviewMat(bool ok);

	bool CalcPlacePos(PaletteWidget::SlotType type,
		Entity* hitEntity,
		const Vec3& hitNormal,
		Vec3& outEntityPos) const;

	bool IsOverlappingCharacter(const Vec3& colCenter, const Vec3& halfExt) const;

	void HandleInput();
	void UpdatePreview();
	void HidePreview();
	bool TryPlaceOnHit(Entity* hitEntity, const Vec3& hitNormal, PaletteWidget::SlotType type);
	bool TryRemoveEntity(Entity* entity);

	// 이전: weak_ptr → lock() 필요
	// 변경: raw ptr 관찰자 — 씬이 수명 관리
	PaletteWidget* _palette   = nullptr;
	Entity*        _character = nullptr;

	bool         _placingMode  = false;
	bool         _previewValid = false;
	std::wstring _savePath     = L"../Saved/scene.xml";

	// 프리뷰 Entity: 씬이 소유, BlockPlacer는 관찰자
	Entity* _previewEntity = nullptr;

	// 이전: shared_ptr<Material> _previewMatOk/_previewMatBad
	// 변경: raw ptr — ResourceManager 또는 로컬 unique_ptr이 소유
	Material* _previewMatOk  = nullptr;
	Material* _previewMatBad = nullptr;

	// 프리뷰용 Material unique_ptr 보관
	std::unique_ptr<Material> _previewMatOkOwned;
	std::unique_ptr<Material> _previewMatBadOwned;

	// 이전: shared_ptr<Shader>
	// 변경: Shader* — ResourceManager에 등록하여 수명 관리
	Shader* _blockShader = nullptr;

	POINT _lastPreviewMouse = { -1, -1 };
	bool  _previewDirty     = true;

	// 이전: array<shared_ptr<Model>, N>
	// 변경: array<unique_ptr<Model>, N>
	std::array<std::unique_ptr<Model>,
		static_cast<int>(PaletteWidget::SlotType::Count)> _modelCache;

	std::vector<std::pair<int32, int32>>  _placedCells;

	// 이전: unordered_set<shared_ptr<Entity>> → erase 시 refcount 감소
	// 변경: unordered_set<Entity*> — 씬이 소유, BlockPlacer는 관찰자
	std::unordered_set<Entity*> _blockSet;
};
