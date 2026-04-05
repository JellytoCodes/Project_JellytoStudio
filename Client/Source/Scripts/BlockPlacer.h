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
    // 콜라이더 규격
    enum class ColliderSize : uint8
    {
        Small,  // 0.5 × 0.5 × 0.5
        Unit,   // 1.0 × 1.0 × 1.0
        Tall,   // 1.0 × 1.5 × 1.0
        Wide,   // 2.0 × 0.5 × 1.0
    };
    static Vec3  GetHalfExtents(ColliderSize s);
    static float GetFullHeight(ColliderSize s) { return GetHalfExtents(s).y * 2.f; }

    BlockPlacer();
    virtual ~BlockPlacer() = default;

    virtual void Awake()      override;
    virtual void Start()      override {}
    virtual void Update()     override;
    virtual void LateUpdate() override {}
    virtual void OnDestroy()  override;

    void SetPalette(std::shared_ptr<PaletteWidget> palette) { _palette = palette; }
    void SetSavePath(const std::wstring& path) { _savePath = path; }
    void SetCharacterEntity(std::shared_ptr<Entity> character) { _character = character; }

    bool IsPlacingMode() const { return _placingMode; }
    void SetPlacingMode(bool on);

    // IBlockPlacer (직렬화 인터페이스 — 위치 기반)
    virtual const std::vector<std::pair<int32, int32>>& GetPlacedBlocks() const override { return _placedCells; }
    virtual bool PlaceBlock(int32 col, int32 row) override { return false; } // 미사용
    virtual void ClearAllBlocks() override;

    // 직접 월드 좌표 기반 배치 (씬 초기화용)
    bool PlaceBlockAt(const Vec3& entityPos, PaletteWidget::SlotType type);

private:
    // 슬롯별 배치 파라미터
    struct MapModelParams
    {
        std::wstring        modelName;
        ColliderSize        collider;
        Vec3                modelScale;
        CollisionChannel    ownChannel;    // 이 블록의 채널
        uint8               pickableMask;  // 이 블록을 피킹할 수 있는 채널
        uint8               faceMask;      // 허용 배치 면 (PlaceFace 비트 OR)
    };

    MapModelParams GetModelParams(PaletteWidget::SlotType type) const;

    std::shared_ptr<Model>    GetOrLoadModel(PaletteWidget::SlotType type);
    std::shared_ptr<Material> GetPreviewMat(bool ok);

    bool CalcPlacePos(PaletteWidget::SlotType type,
        const std::shared_ptr<Entity>& hitEntity,
        const Vec3& hitNormal,
        Vec3& outEntityPos) const;

    bool IsOverlappingCharacter(const Vec3& colCenter, const Vec3& halfExt) const;

    void HandleInput();
    void UpdatePreview();
    void HidePreview();
    bool TryPlaceOnHit(const std::shared_ptr<Entity>& hitEntity,
        const Vec3& hitNormal,
        PaletteWidget::SlotType type);
    bool TryRemoveEntity(const std::shared_ptr<Entity>& entity);

    std::weak_ptr<PaletteWidget> _palette;
    std::weak_ptr<Entity>        _character;

    bool         _placingMode = false;
    bool         _previewValid = false;
    std::wstring _savePath = L"../Saved/scene.xml";

    std::shared_ptr<Entity>   _previewEntity;
    std::shared_ptr<Material> _previewMatOk;
    std::shared_ptr<Material> _previewMatBad;

    // ── 공유 Shader 캐시 ─────────────────────────────────────────
    // PlaceBlockAt() 마다 new Shader() 하면 shader.get()이 블록마다 달라
    // InstanceID = (model, shader) 가 모두 달라져 → 인스턴싱 0%
    // 한 개 공유하면 같은 모델 블록들이 같은 InstanceID → 1 DrawCall
    std::shared_ptr<Shader> _blockShader;

    // 프리뷰 마우스 델타 캐시
    // 마우스가 안 움직이면 PickBlock(O(N)) 재실행 스킵
    POINT _lastPreviewMouse = { -1, -1 };
    bool  _previewDirty = true;

    std::array<std::shared_ptr<Model>,
        static_cast<int>(PaletteWidget::SlotType::Count)> _modelCache;

    // IBlockPlacer 호환용 (사용 안 하지만 인터페이스 충족)
    std::vector<std::pair<int32, int32>> _placedCells;
    // 배치된 블록 엔티티 추적 (Eraser용)
    std::unordered_set<std::shared_ptr<Entity>> _blockSet;
};