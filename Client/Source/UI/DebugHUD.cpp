#include "pch.h"
#include "DebugHUD.h"

#include "Entity/Components/Camera.h"

void DebugHUD::Init(Camera* camera)
{
    _pCamera = camera;
    _visible = false;
}
