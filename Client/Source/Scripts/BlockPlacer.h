#pragma once
#include "Entity/Components/MonoBehaviour.h"
#include "Scene/BlockPlacerInterface.h"
#include "UI/PaletteWidget.h"

class TileMap;
class Material;

// ── BlockPlacer ───────────────────────────────────────────────────────────
// 팔레트 슬롯에 따라 다른 오브젝트를 그리드에 배치/제거
// PaletteWidget의 현재 슬롯을 매 프레임 참조
//
// 단축키:
//   Tab        — 배치 모드 On/Off (PaletteWidget과 동기화)
//   좌클릭     — 현재 슬롯 오브젝트 배치 (Eraser 슬롯이면 제거)
//   우클릭     — 제거
//   Ctrl+S/L   — 씬 저장/로드
class BlockPlacer : public MonoBehaviour, public IBlockPlacer
{
public:
    BlockPlacer();
    virtual ~BlockPlacer() = default;

    virtual void Awake()      override {}
    virtual void Start()      override {}
    virtual void Update()     override;
    virtual void LateUpdate() override {}
    virtual void OnDestroy()  override;

    // 팔레트 연결 (MainApp에서 주입)
    void SetPalette(std::shared_ptr<PaletteWidget> palette) { _palette = palette; }
    void SetSavePath(const std::wstring& path) { _savePath = path; }

    // 기본 블록 머티리얼 (슬롯별로 없으면 이걸 사용)
    void SetDefaultMaterial(std::shared_ptr<Material> mat) { _defaultMat = mat; }

    bool IsPlacingMode() const { return _placingMode; }
    void SetPlacingMode(bool on);

    // ── IBlockPlacer ───────────────────────────────────────────
    virtual const std::vector<std::pair<int32, int32>>& GetPlacedBlocks() const override { return _placedCells; }
    virtual bool PlaceBlock(int32 col, int32 row) override;
    virtual void ClearAllBlocks() override;

    bool RemoveBlock(int32 col, int32 row);

private:
    // 슬롯 타입별 배치 파라미터
    struct PlaceParams
    {
        std::wstring meshKey;   // "Cube" or "Sphere"
        Vec3         scale;     // 오브젝트 스케일
        float        yOffset;   // 배치 높이 오프셋 (중심 Y)
        Vec3         extents;   // AABB extents
    };
    PlaceParams GetPlaceParams(PaletteWidget::SlotType type) const;
    std::shared_ptr<Material> GetSlotMaterial(PaletteWidget::SlotType type);

    void HandleInput();
    void UpdatePreview();
    void HidePreview();
    bool TryPlace(const Vec3& worldPos);
    bool TryRemove(const Vec3& worldPos);
    bool IsCellOccupied(int32 col, int32 row) const;
    std::shared_ptr<TileMap> FindTileMap() const;

    // 팔레트 참조
    std::weak_ptr<PaletteWidget> _palette;

    bool         _placingMode = false;
    bool         _previewValid = false;
    std::wstring _savePath = L"../Saved/scene.xml";

    // 프리뷰
    std::shared_ptr<Entity>   _previewEntity;
    std::shared_ptr<Material> _previewMatOk;
    std::shared_ptr<Material> _previewMatBad;
    std::shared_ptr<Material> _defaultMat;

    // 슬롯별 머티리얼 캐시 (지연 생성)
    std::array<std::shared_ptr<Material>,
        static_cast<int>(PaletteWidget::SlotType::Count)> _slotMats;

    // 블록 데이터
    std::vector<std::pair<int32, int32>>                  _placedCells;
    std::unordered_map<uint64, std::shared_ptr<Entity>>  _blockEntities;
};