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

Camera::~Camera()
{
	
}

void Camera::Awake()
{
	
}

void Camera::Start()
{
	
}

void Camera::LateUpdate()
{
	
}

void Camera::OnDestroy()
{
	
}

void Camera::Update()
{
    UpdateMatrix();

    const Vec3  curPos = GetTransform()->GetPosition();
    const float curYaw = GetTransform()->GetRotation().y;

    const float moveDelta = (curPos - _prevCamPos).LengthSquared();
    const float rotDelta = fabsf(curYaw - _prevCamYaw);

    constexpr float kMoveThreshold = 0.0025f;
    constexpr float kRotThreshold = 0.001f;

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

    BoundingFrustum worldFrustum;
    bool frustumValid = false;

    if (_width > 0.f && _height > 0.f)
    {
        BoundingFrustum viewSpaceFrustum;
        BoundingFrustum::CreateFromMatrix(viewSpaceFrustum, _matProjection);

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

    std::vector<Entity*> newForward;
    newForward.reserve(entities.size());

    for (const auto& entity : entities)
    {
        if (IsCulled(entity->GetLayerIndex())) continue;

        const bool hasRenderer =
            entity->GetComponent<MeshRenderer>() != nullptr ||
            entity->GetComponent<ModelRenderer>() != nullptr ||
            entity->GetComponent<ModelAnimator>() != nullptr;
        if (!hasRenderer) continue;

        if (frustumValid)
        {
            if (auto* aabb = entity->GetComponent<AABBCollider>())
            {
                if (worldFrustum.Contains(aabb->GetBoundingBox()) == DirectX::DISJOINT)
                    continue;
            }
        }

        newForward.push_back(entity.get());
    }

    size_t newHash = newForward.size();
    for (Entity* e : newForward)
    {
        newHash ^= std::hash<uintptr_t>{}(reinterpret_cast<uintptr_t>(e))
            + 0x9e3779b97f4a7c15ULL
            + (newHash << 6) + (newHash >> 2);
    }

    if (newHash != _visibilityHash)
    {
        _visibilityHash = newHash;
        _vecForward = std::move(newForward);

        GET_SINGLE(InstancingManager)->SetDirty();
        GET_SINGLE(InstancingManager)->SetMeshDirty();
    }
    _cullStats.totalEntities   = static_cast<uint32>(entities.size());
    _cullStats.visibleEntities = static_cast<uint32>(_vecForward.size());
    _cullStats.culledEntities  = _cullStats.totalEntities - _cullStats.visibleEntities;

    _sortDirty = false;
}

void Camera::RenderForward()
{
    S_MatView = _matView;
    S_MatProjection = _matProjection;
    GET_SINGLE(InstancingManager)->Render(_vecForward);
}