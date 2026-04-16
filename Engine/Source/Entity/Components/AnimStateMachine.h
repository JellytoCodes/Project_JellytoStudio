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

    void RegisterClip(AnimState state, int32 clipIndex);

    void SetState(AnimState state);

    AnimState GetState() const                  { return _currentState; }
    bool      IsState(AnimState state) const    { return _currentState == state; }

    void SetTweenDuration(float sec)            { _tweenDuration = sec; }

private:
    void ApplyClip(AnimState state);

    AnimState       _currentState  = AnimState::Idle;
    float           _tweenDuration = 0.2f;
    int32           _clipMap[static_cast<uint8>(AnimState::Count)];
};