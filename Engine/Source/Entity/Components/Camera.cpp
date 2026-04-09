#include "Framework.h"
#include "Camera.h"

#include "MeshRenderer.h"
#include "Entity/Components/Transform.h"
#include "Graphics/Managers/InstancingManager.h"
#include "Graphics/Model/ModelAnimator.h"
#include "Graphics/Model/ModelRenderer.h"
#include "Scene/SceneManager.h"
#include "Scene/Scene.h"

Matrix Camera::S_MatView       = Matrix::Identity;
Matrix Camera::S_MatProjection = Matrix::Identity;

Camera::Camera()
	: Super(ComponentType::Camera)
{
	_width  = MAIN_WINDOW_WIDTH;
	_height = MAIN_WINDOW_HEIGHT;
}

Camera::~Camera() {}

void Camera::Awake()  {}
void Camera::Start()  {}
void Camera::LateUpdate() {}
void Camera::OnDestroy() {}

void Camera::Update()
{
	UpdateMatrix();
}

void Camera::UpdateMatrix()
{
	Vec3 eyePosition   = GetTransform()->GetPosition();
	Vec3 focusPosition = eyePosition + GetTransform()->GetLook();
	Vec3 upDirection   = GetTransform()->GetUp();

	_matView = ::XMMatrixLookAtLH(eyePosition, focusPosition, upDirection);

	if (_type == ProjectionType::Perspective)
		_matProjection = ::XMMatrixPerspectiveFovLH(_fov, _width / _height, _near, _far);
	else
		_matProjection = ::XMMatrixOrthographicLH(_width, _height, _near, _far);
}

void Camera::SortEntities()
{
	if (!_sortDirty) return;

	Scene* scene = GET_SINGLE(SceneManager)->GetCurrentScene();
	const auto& entities = scene->GetEntities();

	_vecForward.clear();

	for (const auto& entity : entities)
	{
		if (IsCulled(entity->GetLayerIndex())) continue;

		if (entity->GetComponent<MeshRenderer>()   == nullptr
		 && entity->GetComponent<ModelRenderer>()  == nullptr
		 && entity->GetComponent<ModelAnimator>()  == nullptr)
			continue;

		_vecForward.push_back(entity.get());
	}

	_sortDirty = false;
}

void Camera::RenderForward()
{
	S_MatView       = _matView;
	S_MatProjection = _matProjection;

	GET_SINGLE(InstancingManager)->Render(_vecForward);
}
