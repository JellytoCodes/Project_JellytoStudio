#pragma once
#include "Entity/Components/MonoBehaviour.h"
#include "Scene/BlockPlacerInterface.h"

class TileMap;
class Material;

// ── BlockPlacer ───────────────────────────────────────────────────────────
// 메인 뷰포트에서 마우스로 그리드 위에 블록을 배치/제거
// IBlockPlacer 인터페이스 구현 → SceneSerializer와 연동
//
// 단축키:
//   Tab        — 배치 모드 On/Off
//   좌클릭     — 블록 배치
//   우클릭     — 블록 제거
//   Ctrl+S     — 씬 저장 (블록 포함)
//   Ctrl+L     — 씬 로드 (블록 포함)
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

    void SetBlockMaterial(std::shared_ptr<Material> mat) { _blockMat = mat; }
    void SetSavePath(const std::wstring& path) { _savePath = path; }

    bool IsPlacingMode() const { return _placingMode; }
    void SetPlacingMode(bool on);

    // ── IBlockPlacer 구현 ──────────────────────────────────────
    virtual const std::vector<std::pair<int32, int32>>& GetPlacedBlocks() const override
    {
        return _placedCells;
    }

    virtual bool PlaceBlock(int32 col, int32 row) override;
    virtual void ClearAllBlocks()                 override;

    bool RemoveBlock(int32 col, int32 row);

private:
    void HandleInput();
    void UpdatePreview();
    void HidePreview();
    bool TryPlace(const Vec3& worldPos);
    bool TryRemove(const Vec3& worldPos);
    bool IsCellOccupied(int32 col, int32 row) const;

    std::shared_ptr<TileMap> FindTileMap() const;

    // 배치 상태
    bool _placingMode = false;
    bool _previewValid = false;

    // 저장 경로
    std::wstring _savePath = L"../Saved/scene.xml";

    // 프리뷰 Entity
    std::shared_ptr<Entity>   _previewEntity;
    std::shared_ptr<Material> _blockMat;
    std::shared_ptr<Material> _previewMatOk;
    std::shared_ptr<Material> _previewMatBad;

    // 블록 데이터
    std::vector<std::pair<int32, int32>>              _placedCells;
    std::unordered_map<uint64, std::shared_ptr<Entity>> _blockEntities;
};