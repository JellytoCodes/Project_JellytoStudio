#include "Framework.h"
#include "Camera.h"

#include "Entity/Entity.h"
#include "Entity/Components/Transform.h"
#include "Entity/Components/MeshRenderer.h"
#include "Entity/Components/Collider/AABBCollider.h"
#include "Graphics/Model/ModelRenderer.h"
#include "Graphics/Model/ModelAnimator.h"
#include "Graphics/Managers/InstancingManager.h"
#include "Scene/Scene.h"
#include "Scene/SceneManager.h"

Matrix Camera::S_MatView = Matrix::Identity;
Matrix Camera::S_MatProjection = Matrix::Identity;

Camera::Camera() : Super(ComponentType::Camera)
{
    _width = MAIN_WINDOW_WIDTH;
    _height = MAIN_WINDOW_HEIGHT;
}
Camera::~Camera() {}

void Camera::Awake() {}
void Camera::Start() {}
void Camera::LateUpdate() {}
void Camera::OnDestroy() {}

void Camera::Update()
{
    UpdateMatrix();

    // ★ 카메라가 실제로 이동/회전했을 때만 SortEntities 재빌드 플래그 설정
    // IsometricCameraController::ApplyTransform() 이후의 값을 확인
    const Vec3  curPos = GetTransform()->GetPosition();
    const float curYaw = GetTransform()->GetRotation().y;

    const float moveDelta = (curPos - _prevCamPos).LengthSquared();
    const float rotDelta = fabsf(curYaw - _prevCamYaw);

    constexpr float kMoveThreshold = 1e-4f;
    constexpr float kRotThreshold = 1e-4f;

    if (moveDelta > kMoveThreshold || rotDelta > kRotThreshold)
    {
        _sortDirty = true;
        _prevCamPos = curPos;
        _prevCamYaw = curYaw;
    }
}

void Camera::UpdateMatrix()
{
    const Vec3 eye = GetTransform()->GetPosition();
    const Vec3 focus = eye + GetTransform()->GetLook();
    const Vec3 up = GetTransform()->GetUp();

    _matView = ::XMMatrixLookAtLH(eye, focus, up);

    if (_type == ProjectionType::Perspective)
        _matProjection = ::XMMatrixPerspectiveFovLH(_fov, _width / _height, _near, _far);
    else
        _matProjection = ::XMMatrixOrthographicLH(_width, _height, _near, _far);
}

void Camera::SortEntities()
{
    if (!_sortDirty) return;

    Scene* scene = GET_SINGLE(SceneManager)->GetCurrentScene();
    if (!scene) return;

    const auto& entities = scene->GetEntities();
    _vecForward.clear();
    _vecForward.reserve(entities.size());

    // ── 프러스텀을 SortEntities 내 로컬 변수로 매 재빌드 시마다 신선하게 계산
    // _worldFrustum을 멤버로 두지 않음 → Camera 레이아웃 불변, 파생 버그 차단
    BoundingFrustum worldFrustum;
    bool frustumValid = false;

    if (_width > 0.f && _height > 0.f)
    {
        // 투영 행렬 → 뷰 공간 프러스텀 생성
        BoundingFrustum viewSpaceFrustum;
        BoundingFrustum::CreateFromMatrix(viewSpaceFrustum, _matProjection);

        // 뷰 역행렬(= 카메라→월드 변환)로 월드 공간으로 변환
        // XMMatrixInverse로 행렬식을 먼저 확인해 비가역 케이스를 방어
        XMVECTOR det;
        XMMATRIX viewInv = XMMatrixInverse(&det, _matView);
        float detF;
        XMStoreFloat(&detF, det);

        if (fabsf(detF) > 1e-6f)
        {
            viewSpaceFrustum.Transform(worldFrustum, viewInv);
            frustumValid = true;
        }
    }

    for (const auto& entity : entities)
    {
        if (IsCulled(entity->GetLayerIndex())) continue;

        const bool hasRenderer =
            entity->GetComponent<MeshRenderer>() != nullptr ||
            entity->GetComponent<ModelRenderer>() != nullptr ||
            entity->GetComponent<ModelAnimator>() != nullptr;
        if (!hasRenderer) continue;

        // ── Frustum Culling ──────────────────────────────────────────────
        // frustumValid가 false면(초기화 실패 등) 컬링 스킵 → 항상 포함
        // AABBCollider 없는 엔티티(카메라, 조명 등)도 항상 포함
        if (frustumValid)
        {
            if (auto* aabb = entity->GetComponent<AABBCollider>())
            {
                if (worldFrustum.Contains(aabb->GetBoundingBox()) == DirectX::DISJOINT)
                    continue;
            }
        }

        _vecForward.push_back(entity.get());
    }

    // ★ SetDirty를 SortEntities에서 직접 호출하지 않는다.
    //   InstancingManager의 Dirty 상태는 오직 씬 구조 변경(블록 설치/파괴)
    //   시에만 BlockPlacer에서 관리한다.
    //   카메라 이동에 의한 가시 집합 변경은 다음 프레임 Render에서
    //   InstancingManager가 _vecForward 기반으로 자연스럽게 처리.
    _sortDirty = false;
}

void Camera::RenderForward()
{
    S_MatView = _matView;
    S_MatProjection = _matProjection;
    GET_SINGLE(InstancingManager)->Render(_vecForward);
}