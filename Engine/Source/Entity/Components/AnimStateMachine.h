#pragma once
#include "Component.h"

class ModelAnimator;

enum class AnimState : uint8
{
    Idle   = 0,
    Walk   = 1,
    Attack = 2,
    Die    = 3,
    Count
};

// ── AnimStateMachine ──────────────────────────────────────────────────────
// 고정 컴포넌트 슬롯(ComponentType::AnimStateMachine)에 배치
// → GetComponent<AnimStateMachine>()으로 검색 가능
// ModelAnimator의 TweenDesc를 직접 조작해서 상태별 클립 전환
//
// 배치: Engine/Source/Entity/Components/
class AnimStateMachine : public Component
{
    using Super = Component;
public:
    AnimStateMachine();
    virtual ~AnimStateMachine() = default;

    virtual void Awake()      override;
    virtual void Start()      override {}
    virtual void Update()     override {}
    virtual void LateUpdate() override {}
    virtual void OnDestroy()  override {}

    // 상태 → 애니메이션 클립 인덱스 등록 (ReadAnimation 순서와 일치)
    void RegisterClip(AnimState state, int32 clipIndex);

    // 상태 전환 (Die는 되돌릴 수 없음, 같은 상태 무시)
    void SetState(AnimState state);

    AnimState GetState()          const { return _currentState; }
    bool      IsState(AnimState state) const { return _currentState == state; }

    void SetTweenDuration(float sec) { _tweenDuration = sec; }

private:
    void ApplyClip(AnimState state);

    AnimState _currentState  = AnimState::Idle;
    float     _tweenDuration = 0.2f;
    int32     _clipMap[static_cast<uint8>(AnimState::Count)];
};