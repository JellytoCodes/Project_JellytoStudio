#pragma once
#include "Entity/Components/MonoBehaviour.h"
#include "Scene/BlockPlacerInterface.h"
#include "UI/PaletteWidget.h"

class Material;
class TileMap;
class Model;

// ── BlockPlacer ───────────────────────────────────────────────────────────
// MapModel(.mesh/.xml) 기반 오브젝트 배치/제거
// PaletteWidget 슬롯 → MapModel 로드 → ModelRenderer 컴포넌트로 배치
//
// 블록 키: col*1000000 + row*100 + layer (최대 LAYER_MAX 적층)
// 캐릭터 AABB 겹침 시 빨간 프리뷰 + 배치 불가
class BlockPlacer : public MonoBehaviour, public IBlockPlacer
{
public:
    BlockPlacer();
    virtual ~BlockPlacer() = default;

    virtual void Awake()      override;
    virtual void Start()      override {}
    virtual void Update()     override;
    virtual void LateUpdate() override {}
    virtual void OnDestroy()  override;

    void SetPalette(std::shared_ptr<PaletteWidget> palette)    { _palette   = palette;    }
    void SetSavePath(const std::wstring& path)                  { _savePath  = path;       }
    void SetCharacterEntity(std::shared_ptr<Entity> character)  { _character = character;  }

    bool IsPlacingMode() const { return _placingMode; }
    void SetPlacingMode(bool on);

    // IBlockPlacer
    virtual const std::vector<std::pair<int32,int32>>& GetPlacedBlocks() const override { return _placedCells; }
    virtual bool PlaceBlock(int32 col, int32 row) override;
    virtual void ClearAllBlocks() override;
    bool RemoveBlock(int32 col, int32 row);

private:
    static constexpr int32 LAYER_MAX = 8;

    // 슬롯 타입 → 모델 파라미터
    struct MapModelParams
    {
        std::wstring modelName;  // MapModel/XXX.mesh 의 XXX
        float        blockHeight;// 쌓기 계산용 높이 (월드 단위)
        Vec3         extents;    // AABB half-extents
    };
    MapModelParams GetModelParams(PaletteWidget::SlotType type) const;

    // 모델 캐시 (슬롯별 1회 로드 후 공유)
    std::shared_ptr<Model> GetOrLoadModel(PaletteWidget::SlotType type);

    // 프리뷰용 큐브 머티리얼
    std::shared_ptr<Material> GetPreviewMat(bool ok);

    bool FindNextLayer(int32 col, int32 row, float blockH,
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

    // 프리뷰 Entity (큐브로 배치 위치 표시)
    std::shared_ptr<Entity>   _previewEntity;
    std::shared_ptr<Material> _previewMatOk;
    std::shared_ptr<Material> _previewMatBad;

    // 슬롯별 Model 캐시
    std::array<std::shared_ptr<Model>,
        static_cast<int>(PaletteWidget::SlotType::Count)> _modelCache;

    std::vector<std::pair<int32,int32>>                  _placedCells;
    std::unordered_map<uint64, std::shared_ptr<Entity>>  _blockEntities;
};