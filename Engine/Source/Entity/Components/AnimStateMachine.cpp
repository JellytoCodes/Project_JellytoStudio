
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
    if (_currentState == AnimState::Die) return;
    if (_currentState == state)          return;

    _currentState = state;
    ApplyClip(state);
}

void AnimStateMachine::ApplyClip(AnimState state)
{
    Entity* entity = _entity;
    if (entity == nullptr) return;

    ModelAnimator* animator = entity->GetComponent<ModelAnimator>();
    if (animator == nullptr) return;

    int32 clipIdx = _clipMap[static_cast<int>(state)];
    if (clipIdx < 0) return;

    TweenDesc& tween = animator->GetTweenDesc();

    if (tween.curr.animIndex == clipIdx) return;

    tween.next.animIndex = clipIdx;
    tween.next.currFrame = 0;
    tween.next.nextFrame = 1;
    tween.next.sumTime   = 0.f;
    tween.next.ratio     = 0.f;
    tween.tweenSumTime   = 0.f;
    tween.tweenRatio     = 0.f;
    tween.tweenDuration  = _tweenDuration;
}