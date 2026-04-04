#pragma once
#include "Entity/Components/MonoBehaviour.h"

class Model;
class ModelRenderer;

// ── OneBlockScript ────────────────────────────────────────────────────────
// 원블록 챌린지 핵심 스크립트 — 중앙 시작 블록에 부착
//
// 게임 루프:
//   1. 캐릭터가 블록 근처에서 F키 → 채굴
//   2. 블록이 사라지고 2.5초 후 다음 단계 모델로 재생성
//   3. 채굴 시마다 인접 위치에 Priming 블록 드랍 (플레이어가 확장에 사용)
//
// 단계 (Phase) 진행 — 5회 채굴마다 단계 상승:
//   Phase 0 ( 0~ 4회): Priming_01  숲 지대
//   Phase 1 ( 5~ 9회): Priming_02  동굴
//   Phase 2 (10~14회): Priming_03  지하 깊은 곳
//   Phase 3 (15~19회): Bridge      절벽 지대
//   Phase 4 (20~24회): Mushroom_01 버섯 숲
//   Phase 5 (25~29회): Mushroom_02 심층부
//   Phase 6 (30회~  ): Mushroom_03 마지막 차원
//
// 조작: F 키 (캐릭터가 MINE_RANGE 이내일 때)
class OneBlockScript : public MonoBehaviour
{
public:
    OneBlockScript();
    virtual ~OneBlockScript() = default;

    virtual void Awake()      override {}
    virtual void Start()      override;
    virtual void Update()     override;
    virtual void LateUpdate() override {}
    virtual void OnDestroy()  override {}

    void SetCharacterEntity(std::shared_ptr<Entity> ch) { _character = ch; }

    int32 GetTotalBreaks()   const { return _totalBreaks; }
    int32 GetCurrentPhase()  const { return _currentPhase; }
    bool  IsBroken()         const { return _isBroken; }

private:
    // ── 단계 데이터 ──────────────────────────────────────────
    struct PhaseData
    {
        std::wstring modelName;    // MapModel 파일명 (확장자 없음)
        std::wstring dropModel;    // 이 단계 채굴 시 드랍되는 블록 모델명
        std::wstring phaseName;    // 디버그/UI용 단계 이름
        int32        breaksToNext; // 다음 단계까지 필요한 추가 채굴 횟수
    };
    static const std::vector<PhaseData>& GetPhaseTable();

    // ── 핵심 동작 ────────────────────────────────────────────
    void TryMine();       // F키 + 근접 체크 → Mine() 호출
    void Mine();          // 채굴 실행: 블록 숨김 + 드랍
    void Respawn();       // _respawnTimer 만료 → 재생성
    void UpdatePhase();   // _totalBreaks 기반으로 _currentPhase 갱신

    // ── 모델 교체 ────────────────────────────────────────────
    // ModelRenderer::SetModel()로 교체, 셰이더는 공유 인스턴스 재사용
    void ApplyPhaseModel(const std::wstring& modelName);

    // ── 드랍 블록 생성 ────────────────────────────────────────
    // 4방향 중 _totalBreaks % 4 방향에 Priming 콜라이더 블록 스폰
    void SpawnDropBlock(const std::wstring& modelName);

    // ── 유틸 ─────────────────────────────────────────────────
    bool IsCharacterNearby() const;

    // ── 멤버 ─────────────────────────────────────────────────
    std::weak_ptr<Entity> _character;

    int32 _totalBreaks  = 0;
    int32 _currentPhase = 0;

    bool  _isBroken     = false;
    float _respawnTimer = 0.f;

    static constexpr float RESPAWN_DELAY = 2.5f; // 블록 재생성 대기 (초)
    static constexpr float MINE_RANGE    = 3.0f; // 채굴 가능 거리 (월드 단위)

    // 단계별 모델 캐시 (Start()에서 사전 로드)
    std::vector<std::shared_ptr<Model>> _phaseModels;
    std::vector<std::shared_ptr<Model>> _dropModels;

    // 드랍 위치 오프셋 (4방향 순환)
    static const Vec3 s_dropOffsets[4];
};