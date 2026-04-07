
#include "Framework.h"
#include "AnimStateMachine.h"

#include "Entity/Entity.h"
#include "Graphics/Model/ModelAnimator.h"

AnimStateMachine::AnimStateMachine()
    : Super(ComponentType::AnimStateMachine)
{
    for (int i = 0; i < static_cast<int>(AnimState::Count); i++)
        _clipMap[i] = -1;
}

void AnimStateMachine::Awake()
{
    ApplyClip(_currentState);
}

void AnimStateMachine::RegisterClip(AnimState state, int32 clipIndex)
{
    int i = static_cast<int>(state);
    if (i >= 0 && i < static_cast<int>(AnimState::Count))
        _clipMap[i] = clipIndex;
}

void AnimStateMachine::SetState(AnimState state)
{
    if (_currentState == AnimState::Die) return; // 사망 불가역
    if (_currentState == state)          return; // 동일 상태 무시

    _currentState = state;
    ApplyClip(state);
}

void AnimStateMachine::ApplyClip(AnimState state)
{
    auto entity = _entity.lock();
    if (!entity) return;

    auto animator = entity->GetComponent<ModelAnimator>();
    if (!animator) return;

    int32 clipIdx = _clipMap[static_cast<int>(state)];
    if (clipIdx < 0) return; // 미등록

    TweenDesc& tween = animator->GetTweenDesc();

    // 이미 재생 중인 클립이면 무시
    if (tween.curr.animIndex == clipIdx) return;

    // next 슬롯으로 블렌딩 전환
    tween.next.animIndex = clipIdx;
    tween.next.currFrame = 0;
    tween.next.nextFrame = 1;
    tween.next.sumTime   = 0.f;
    tween.next.ratio     = 0.f;
    tween.tweenSumTime   = 0.f;
    tween.tweenRatio     = 0.f;
    tween.tweenDuration  = _tweenDuration;
}