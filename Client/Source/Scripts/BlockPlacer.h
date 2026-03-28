#pragma once
#include "Entity/Components/MonoBehaviour.h"
#include "Scene/BlockPlacerInterface.h"
#include "UI/PaletteWidget.h"

class TileMap;
class Material;

// ── BlockPlacer ───────────────────────────────────────────────────────────
// 팔레트 슬롯에 따라 다른 오브젝트를 그리드에 배치/제거
//
// 충돌 처리:
//   캐릭터 AABB 겹침 → 빨간 프리뷰 + 배치 불가
//   블록 위에 배치   → Y축으로 자동 적층 (layer++)
//
// 블록 키: col * 1000000 + row * 100 + layer
//   layer 0 = 바닥(Y=0), layer 1 = 블록 위, ...
//   같은 col/row에 최대 LAYER_MAX개까지 쌓기 가능
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

    void SetPalette(std::shared_ptr<PaletteWidget> palette)   { _palette = palette; }
    void SetSavePath(const std::wstring& path)                { _savePath = path;   }
    void SetDefaultMaterial(std::shared_ptr<Material> mat)    { _defaultMat = mat;  }

    // 캐릭터 Entity 등록 (겹침 검사용)
    void SetCharacterEntity(std::shared_ptr<Entity> character){ _character = character; }

    bool IsPlacingMode() const { return _placingMode; }
    void SetPlacingMode(bool on);

    // IBlockPlacer
    virtual const std::vector<std::pair<int32,int32>>& GetPlacedBlocks() const override { return _placedCells; }
    virtual bool PlaceBlock(int32 col, int32 row) override;
    virtual void ClearAllBlocks() override;

    bool RemoveBlock(int32 col, int32 row);

private:
    static constexpr int32 LAYER_MAX = 8; // 최대 쌓기 레이어

    struct PlaceParams
    {
        std::wstring meshKey;
        Vec3         scale;
        float        blockHeight; // Y방향 실제 높이 (쌓기 계산용)
        Vec3         extents;
    };
    PlaceParams GetPlaceParams(PaletteWidget::SlotType type) const;
    std::shared_ptr<Material> GetSlotMaterial(PaletteWidget::SlotType type);

    // 해당 col/row에서 다음 빈 레이어와 Y위치 반환 (쌓기 지원)
    // 반환값: 배치 가능하면 true, layer/yPos에 결과 기록
    bool FindNextLayer(int32 col, int32 row, const PlaceParams& params,
                       int32& outLayer, float& outY) const;

    // 캐릭터 AABB와 지정 월드 위치+크기가 겹치는지 검사
    bool IsOverlappingCharacter(const Vec3& center, const Vec3& halfExtents) const;

    void HandleInput();
    void UpdatePreview();
    void HidePreview();
    bool TryPlace(const Vec3& worldPos);
    bool TryRemove(const Vec3& worldPos);
    bool IsCellLayerOccupied(int32 col, int32 row, int32 layer) const;
    bool IsCellOccupied(int32 col, int32 row) const; // 레이어 0 기준
    std::shared_ptr<TileMap> FindTileMap() const;

    // 블록 키: col*1000000 + row*100 + layer
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
    std::shared_ptr<Material> _defaultMat;

    std::array<std::shared_ptr<Material>,
        static_cast<int>(PaletteWidget::SlotType::Count)> _slotMats;

    // (col, row) 목록 — 직렬화용 (중복 허용, 같은 col/row에 여러 레이어)
    std::vector<std::pair<int32,int32>> _placedCells;
    // key → Entity
    std::unordered_map<uint64, std::shared_ptr<Entity>> _blockEntities;
};