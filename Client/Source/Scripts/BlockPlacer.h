
#pragma once
#include "Entity/Components/MonoBehaviour.h"
#include "Scene/BlockPlacerInterface.h"
#include "Entity/Components/Collider/CollisionChannel.h"
#include "UI/PaletteWidget.h"

class Material;
class Model;
class Shader;
class InventoryData;

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

    // ── 외부 의존성 주입 ────────────────────────────────────────
    void SetPalette(PaletteWidget* palette)          { _palette    = palette; }
    void SetSavePath(const std::wstring& path)       { _savePath   = path; }
    void SetCharacterEntity(Entity* character)       { _character  = character; }
    void SetInventoryData(InventoryData* inventory)  { _pInventory = inventory; } // ★ 추가

    bool IsPlacingMode() const { return _placingMode; }
    void SetPlacingMode(bool on);

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

    std::shared_ptr<Model>    GetOrLoadModel(PaletteWidget::SlotType type);
    std::shared_ptr<Material> GetPreviewMat(bool ok);

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

    // ── 기존 멤버 ────────────────────────────────────────────────
    PaletteWidget* _palette       = nullptr;
    Entity*        _character     = nullptr;
    Entity*        _previewEntity = nullptr;

    bool         _placingMode  = false;
    bool         _previewValid = false;
    std::wstring _savePath     = L"../Saved/scene.xml";

    std::shared_ptr<Material> _previewMatOk;
    std::shared_ptr<Material> _previewMatBad;
    std::shared_ptr<Shader>   _blockShader;

    std::array<std::shared_ptr<Model>, static_cast<int>(PaletteWidget::SlotType::Count)> _modelCache;

    POINT _lastPreviewMouse = { -1, -1 };
    bool  _previewDirty     = true;

    std::vector<std::pair<int32, int32>> _placedCells;
    std::unordered_set<Entity*>          _blockSet;

    InventoryData* _pInventory = nullptr;

    std::unordered_map<Entity*, PaletteWidget::SlotType> _blockTypeMap; // ★ 추가
};