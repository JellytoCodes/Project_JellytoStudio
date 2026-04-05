#include "Framework.h"
#include "Camera.h"

#include "MeshRenderer.h"
#include "Entity/Components/Transform.h"
#include "Graphics/Managers/InstancingManager.h"
#include "Graphics/Model/ModelAnimator.h"
#include "Graphics/Model/ModelRenderer.h"
#include "Scene/SceneManager.h"

Matrix Camera::S_MatView = Matrix::Identity;
Matrix Camera::S_MatProjection = Matrix::Identity;

Camera::Camera()
	: Super(ComponentType::Camera)
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

void Camera::Update()
{
	UpdateMatrix();
}

void Camera::LateUpdate()
{
	
}

void Camera::OnDestroy()
{
	
}

void Camera::UpdateMatrix()
{
	Vec3 eyePosition = GetTransform()->GetPosition();
	Vec3 focusPosition = eyePosition + GetTransform()->GetLook();
	Vec3 upDirection = GetTransform()->GetUp();

	_matView = ::XMMatrixLookAtLH(eyePosition, focusPosition, upDirection);

	if (_type == ProjectionType::Perspective)
		_matProjection = ::XMMatrixPerspectiveFovLH(_fov, _width / _height, _near, _far);

	else
		_matProjection = ::XMMatrixOrthographicLH(_width, _height, _near, _far);
}

void Camera::SortEntities()
{
	// ── Dirty 플래그 캐시 ──────────────────────────────────────────
	// Scene::Add/Remove 가 SetSortDirty()를 호출할 때만 재빌드
	// 효과: 블록 배치/제거 없는 평상시엔 GetComponent 호출 0회/프레임
	if (!_sortDirty) return;

	std::shared_ptr<Scene> scene = GET_SINGLE(SceneManager)->GetCurrentScene();
	std::unordered_set<std::shared_ptr<Entity>>& entities = scene->GetEntities();

	_vecForward.clear();

	for (auto& entity : entities)
	{
		if (IsCulled(entity->GetLayerIndex())) continue;

		if (entity->GetComponent<MeshRenderer>() == nullptr
			&& entity->GetComponent<ModelRenderer>() == nullptr
			&& entity->GetComponent<ModelAnimator>() == nullptr)
			continue;

		_vecForward.push_back(entity);
	}

	_sortDirty = false;
}

void Camera::RenderForward()
{
	S_MatView = _matView;
	S_MatProjection = _matProjection;

	GET_SINGLE(InstancingManager)->Render(_vecForward);

}