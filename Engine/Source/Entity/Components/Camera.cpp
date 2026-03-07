
#include "Framework.h"
#include "Camera.h"
#include "Entity/Components/Transform.h"

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

void Camera::RenderForward()
{
	S_MatView = _matView;
	S_MatProjection = _matProjection;
}
