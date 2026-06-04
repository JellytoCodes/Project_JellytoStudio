#pragma once

class Camera;

class DebugHUD
{
public:
    void Init(Camera* camera);

    bool IsVisible() const { return _visible; }

private:
    Camera* _pCamera = nullptr;
    bool    _visible = false;
};
