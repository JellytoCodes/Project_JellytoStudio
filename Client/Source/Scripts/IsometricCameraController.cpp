#include "pch.h"
#include "IsometricCameraController.h"

#include "Entity/Entity.h"
#include "Entity/Components/Transform.h"
#include "Core/Managers/InputManager.h"
#include "Core/Managers/TimeManager.h"

IsometricCameraController::IsometricCameraController()
{
}

void IsometricCameraController::Awake()
{
    ApplyTransform();
}

void IsometricCameraController::Start()
{
}

void IsometricCameraController::Update()
{
    float dt = GET_SINGLE(TimeManager)->GetDeltaTime();

    // 타깃 추적
    auto target = _target.lock();
    if (target && target->GetTransform())
        _pivot = target->GetTransform()->GetPosition();

    HandleOrbit(dt);
    HandleZoom(dt);
    HandlePan(dt);
    ApplyTransform();
}

void IsometricCameraController::LateUpdate() {}
void IsometricCameraController::OnDestroy()  {}

// ── 패닝 ─────────────────────────────────────────────────────────────────
void IsometricCameraController::HandlePan(float dt)
{
    if (!_target.expired()) return; // 타깃 추적 중엔 패닝 없음

    auto input = GET_SINGLE(InputManager);

    // Yaw 기준 수평 이동 벡터 계산
    // LH에서 yaw 양수 = Y축 시계방향
    // 기본 Forward = +Z, yaw 적용 후:
    //   forward_xz = (sin(yaw), 0, cos(yaw))
    //   right_xz   = (cos(yaw), 0, -sin(yaw))
    float yawRad = XMConvertToRadians(_yawDeg);
    Vec3 forward( sinf(yawRad), 0.f,  cosf(yawRad));
    Vec3 right  ( cosf(yawRad), 0.f, -sinf(yawRad));

    Vec3 move = Vec3::Zero;
    if (input->GetButton(KEY_TYPE::W) || input->GetButton(KEY_TYPE::UP))    move += forward;
    if (input->GetButton(KEY_TYPE::S) || input->GetButton(KEY_TYPE::DOWN))  move -= forward;
    if (input->GetButton(KEY_TYPE::D) || input->GetButton(KEY_TYPE::RIGHT)) move += right;
    if (input->GetButton(KEY_TYPE::A) || input->GetButton(KEY_TYPE::LEFT))  move -= right;

    if (move.LengthSquared() > 0.f)
    {
        move.Normalize();
        _pivot += move * _panSpeed * dt;
    }
}

// ── 줌 ───────────────────────────────────────────────────────────────────
void IsometricCameraController::HandleZoom(float dt)
{
    auto input = GET_SINGLE(InputManager);
    if (input->GetButton(KEY_TYPE::Z)) _distance += _zoomSpeed * dt;
    if (input->GetButton(KEY_TYPE::C)) _distance -= _zoomSpeed * dt;
    _distance = max(_minDist, std::min(_maxDist, _distance));
}

// ── 궤도 회전 ─────────────────────────────────────────────────────────────
void IsometricCameraController::HandleOrbit(float dt)
{
    auto input = GET_SINGLE(InputManager);
    if (input->GetButton(KEY_TYPE::Q)) _yawDeg -= _orbitSpeed * dt;
    if (input->GetButton(KEY_TYPE::E)) _yawDeg += _orbitSpeed * dt;

    if (_yawDeg >= 360.f) _yawDeg -= 360.f;
    if (_yawDeg <    0.f) _yawDeg += 360.f;
}

// ── Transform 반영 ────────────────────────────────────────────────────────
void IsometricCameraController::ApplyTransform()
{
    auto transform = GetTransform();
    if (!transform) return;

    float pitchRad = XMConvertToRadians(_pitchDeg);
    float yawRad   = XMConvertToRadians(_yawDeg);

    // ── 카메라가 바라보는 방향 벡터 계산 ──────────────────────────────
    // Transform이 Rx * Ry 순으로 회전을 적용하므로
    // Look 방향 = Ry(yaw) * Rx(pitch) * (0, 0, 1) 로 계산
    //
    // Rx(pitch): (0,0,1) → (0, -sin(p), cos(p))
    // Ry(yaw)  : (x,y,z) → (x*cos(y)+z*sin(y), y, -x*sin(y)+z*cos(y))
    //
    // pitch 양수 = 앞으로 숙임 = Look의 Y 성분이 음수(아래 방향)
    float cp = cosf(pitchRad);
    float sp = sinf(pitchRad);
    float cy = cosf(yawRad);
    float sy = sinf(yawRad);

    // Rx 적용: (0, -sp, cp)
    // Ry 적용:
    Vec3 lookDir(
         cp * sy,   // x = cp*sy
        -sp,        // y = -sp
         cp * cy    // z = cp*cy
    );
    lookDir.Normalize();

    // 카메라 위치 = 피벗에서 lookDir 반대 방향으로 distance만큼
    Vec3 camPos = _pivot - lookDir * _distance;

    // ── Transform 설정 ────────────────────────────────────────────────
    transform->SetLocalPosition(camPos);

    // 회전은 오일러 그대로 (Rx * Ry 순)
    // pitch 양수 = X축 회전 = Look이 아래를 향함
    transform->SetLocalRotation(Vec3(pitchRad, yawRad, 0.f));
}