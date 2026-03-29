#pragma once
#include "Entity/Components/MonoBehaviour.h"
#include "Scene/BlockPlacerInterface.h"
#include "UI/PaletteWidget.h"

class Material;
class TileMap;
class Model;

// ── BlockPlacer ───────────────────────────────────────────────────────────
// MapModel 기반 오브젝트 그리드 배치 시스템
//
// 배치 구조:
//   Entity (Collider 규격 기준 Transform)
//     ├ AABBCollider  — 규격 박스 크기 그대로
//     └ ModelRenderer — modelScale 보정으로 규격에 맞춰 렌더
//
// 콜라이더 규격 (ColliderSize):
//   Small  : 0.5 x 0.5 x 0.5  (1x1 그리드, 낮은 오브젝트)
//   Unit   : 1.0 x 1.0 x 1.0  (1x1 그리드, 1칸 높이)
//   Tall   : 1.0 x 1.5 x 1.0  (1x1 그리드, 키 큰 오브젝트)
//   Wide   : 2.0 x 0.5 x 1.0  (2x1 그리드, 낮고 넓은 브릿지 등)
class BlockPlacer : public MonoBehaviour, public IBlockPlacer
{
public:
    // 미리 정의된 콜라이더 규격
    enum class ColliderSize : uint8
    {
        Small,   // half-extents (0.25, 0.25, 0.25) → 0.5 x 0.5 x 0.5 박스
        Unit,    // half-extents (0.5,  0.5,  0.5)  → 1.0 x 1.0 x 1.0 박스
        Tall,    // half-extents (0.5,  0.75, 0.5)  → 1.0 x 1.5 x 1.0 박스
        Wide,    // half-extents (1.0,  0.25, 0.5)  → 2.0 x 0.5 x 1.0 박스
    };

    // 규격 → half-extents 반환
    static Vec3 GetColliderHalfExtents(ColliderSize size)
    {
        switch (size)
        {
        case ColliderSize::Small: return Vec3(0.25f, 0.25f, 0.25f);
        case ColliderSize::Unit:  return Vec3(0.5f,  0.5f,  0.5f);
        case ColliderSize::Tall:  return Vec3(0.5f,  0.75f, 0.5f);
        case ColliderSize::Wide:  return Vec3(1.0f,  0.25f, 0.5f);
        default:                  return Vec3(0.5f,  0.5f,  0.5f);
        }
    }

    // 규격 → 박스 전체 높이 (적층 계산용)
    static float GetColliderFullHeight(ColliderSize size)
    {
        return GetColliderHalfExtents(size).y * 2.f;
    }

    BlockPlacer();
    virtual ~BlockPlacer() = default;

    virtual void Awake()      override;
    virtual void Start()      override {}
    virtual void Update()     override;
    virtual void LateUpdate() override {}
    virtual void OnDestroy()  override;

    void SetPalette(std::shared_ptr<PaletteWidget> palette)   { _palette   = palette;   }
    void SetSavePath(const std::wstring& path)                { _savePath  = path;      }
    void SetCharacterEntity(std::shared_ptr<Entity> character){ _character = character; }

    bool IsPlacingMode() const { return _placingMode; }
    void SetPlacingMode(bool on);

    // IBlockPlacer
    virtual const std::vector<std::pair<int32,int32>>& GetPlacedBlocks() const override { return _placedCells; }
    virtual bool PlaceBlock(int32 col, int32 row) override;
    virtual void ClearAllBlocks() override;
    bool RemoveBlock(int32 col, int32 row);

private:
    static constexpr int32 LAYER_MAX = 8;

    // 슬롯 → 모델 + 콜라이더 + 스케일 정의
    struct MapModelParams
    {
        std::wstring  modelName;     // Models/MapModel/{modelName}.mesh
        ColliderSize  collider;      // 배치 규격 박스
        Vec3          modelScale;    // 모델을 규격 박스에 맞추는 시각 스케일
    };
    MapModelParams GetModelParams(PaletteWidget::SlotType type) const;

    std::shared_ptr<Model> GetOrLoadModel(PaletteWidget::SlotType type);
    std::shared_ptr<Material> GetPreviewMat(bool ok);

    // 다음 빈 레이어 탐색 (이 슬롯의 콜라이더 높이 기준으로 y 계산)
    bool FindNextLayer(int32 col, int32 row, ColliderSize size,
                       int32& outLayer, float& outY) const;
    bool IsOverlappingCharacter(const Vec3& center, const Vec3& halfExtents) const;

    void HandleInput();
    void UpdatePreview();
    void HidePreview();
    bool TryPlace(const Vec3& worldPos);
    bool TryRemove(const Vec3& worldPos);
    bool IsCellLayerOccupied(int32 col, int32 row, int32 layer) const;
    bool IsCellOccupied(int32 col, int32 row) const;
    std::shared_ptr<TileMap> FindTileMap() const;

    static uint64 MakeKey(int32 col, int32 row, int32 layer)
    { return (uint64)col * 1000000 + (uint64)row * 100 + (uint64)layer; }

    std::weak_ptr<PaletteWidget> _palette;
    std::weak_ptr<Entity>        _character;

    bool         _placingMode  = false;
    bool         _previewValid = false;
    std::wstring _savePath     = L"../Saved/scene.xml";

    std::shared_ptr<Entity>   _previewEntity;
    std::shared_ptr<Material> _previewMatOk;
    std::shared_ptr<Material> _previewMatBad;

    std::array<std::shared_ptr<Model>,
        static_cast<int>(PaletteWidget::SlotType::Count)> _modelCache;

    std::vector<std::pair<int32,int32>>                  _placedCells;
    std::unordered_map<uint64, std::shared_ptr<Entity>>  _blockEntities;
};