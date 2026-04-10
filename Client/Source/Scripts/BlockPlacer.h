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

	// Scene이 Entity 수명 관리 → raw pointer (observer)
	void SetPalette(PaletteWidget* palette) { _palette = palette; }
	void SetSavePath(const std::wstring& path) { _savePath = path; }
	void SetCharacterEntity(Entity* character) { _character = character; }

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

	// ── 리소스 API 정합성 ─────────────────────────────────────────────────
	// ModelRenderer::SetModel(shared_ptr<Model>),
	// MeshRenderer::SetMaterial(shared_ptr<Material>),
	// ModelRenderer(shared_ptr<Shader>, bool) 모두 shared_ptr 요구 →
	// 리소스 타입은 shared_ptr 유지.  Entity/Component 참조만 raw ptr.
	std::shared_ptr<Model>    GetOrLoadModel(PaletteWidget::SlotType type);
	std::shared_ptr<Material> GetPreviewMat(bool ok);

	// Entity/Component 참조: Scene 소유, BlockPlacer는 observer
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

	// ── Entity/Component 참조 (raw pointer, observer) ─────────────────────
	PaletteWidget* _palette = nullptr;   // Scene 소유
	Entity* _character = nullptr;   // Scene 소유
	Entity* _previewEntity = nullptr; // Scene 소유, 관찰자

	bool         _placingMode = false;
	bool         _previewValid = false;
	std::wstring _savePath = L"../Saved/scene.xml";

	// ── 리소스 (shared_ptr) ───────────────────────────────────────────────
	// Material: 이중소유(raw ptr + unique_ptr) 패턴 제거 → shared_ptr로 통일.
	// unique_ptr<Material>은 forward-declare와 함께 쓰면
	// static_assert "can't delete an incomplete type" 발생.
	std::shared_ptr<Material> _previewMatOk;
	std::shared_ptr<Material> _previewMatBad;

	// Shader: ModelRenderer 생성자가 shared_ptr<Shader>를 요구하므로 shared_ptr 보관.
	// ResourceManager는 Shader/Model 타입을 지원하지 않으므로 직접 멤버로 관리.
	std::shared_ptr<Shader> _blockShader;

	// Model 캐시: ModelRenderer::SetModel(shared_ptr<Model>) → shared_ptr 유지
	std::array<std::shared_ptr<Model>, static_cast<int>(PaletteWidget::SlotType::Count)> _modelCache;

	POINT _lastPreviewMouse = { -1, -1 };
	bool  _previewDirty = true;

	std::vector<std::pair<int32, int32>> _placedCells;

	// 배치된 블록 관찰 목록: Scene 소유, BlockPlacer는 observer
	std::unordered_set<Entity*> _blockSet;
};